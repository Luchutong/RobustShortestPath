#include "rsp/baseline.hpp"

#include "rsp/bellman.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <cmath>
#include <functional>
#include <queue>
#include <vector>

namespace rsp {
namespace {

struct DeterministicEdge {
    int from = -1;
    int to = -1;
    int action = -1;
    double cost = 0.0;
};

bool has_negative_transition_cost(const RobustGraph& graph) {
    for (const auto& node : graph.nodes) {
        for (const auto& action : node.actions) {
            for (const auto& tr : action.trans) {
                if (tr.cost < -EPS) {
                    return true;
                }
            }
        }
    }
    return false;
}

Transition worst_immediate_successor(const Action& action) {
    Transition chosen = action.trans.front();
    for (const auto& tr : action.trans) {
        if (tr.cost > chosen.cost + EPS ||
            (std::abs(tr.cost - chosen.cost) <= EPS && tr.to < chosen.to)) {
            chosen = tr;
        }
    }
    return chosen;
}

std::vector<DeterministicEdge> build_deterministic_edges(
    const RobustGraph& graph,
    DeterministicMode mode
) {
    std::vector<DeterministicEdge> edges;
    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }
        for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
            const auto& action = graph.nodes[x].actions[a];
            if (mode == DeterministicMode::BestCase) {
                for (const auto& tr : action.trans) {
                    edges.push_back({x, tr.to, a, tr.cost});
                }
            } else if (mode == DeterministicMode::WorstImmediate) {
                const auto tr = worst_immediate_successor(action);
                edges.push_back({x, tr.to, a, tr.cost});
            } else {
                const auto& tr = action.trans.front();
                edges.push_back({x, tr.to, a, tr.cost});
            }
        }
    }
    return edges;
}

bool better_shortest_path_choice(
    double candidate,
    const DeterministicEdge& edge,
    double best_distance,
    int best_action,
    int best_next
) {
    if (less_with_eps(candidate, best_distance)) {
        return true;
    }
    if (std::abs(candidate - best_distance) > EPS) {
        return false;
    }
    if (best_action < 0 || edge.action < best_action) {
        return true;
    }
    return edge.action == best_action && edge.to < best_next;
}

}  // namespace

BaselineResult deterministic_dijkstra_baseline(
    const RobustGraph& graph,
    DeterministicMode mode
) {
    graph.validate();
    BaselineResult result;
    result.policy.assign(graph.n, -1);
    result.value.assign(graph.n, INF);
    if (has_negative_transition_cost(graph)) {
        return result;
    }

    std::vector<std::vector<DeterministicEdge>> incoming(graph.n);
    for (const auto& edge : build_deterministic_edges(graph, mode)) {
        incoming[edge.to].push_back(edge);
    }

    std::vector<int> best_next(graph.n, -1);
    using QueueItem = std::pair<double, int>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> pq;
    result.value[graph.terminal] = 0.0;
    pq.push({0.0, graph.terminal});

    while (!pq.empty()) {
        const auto [dist, node] = pq.top();
        pq.pop();
        if (std::abs(dist - result.value[node]) > EPS) {
            continue;
        }
        for (const auto& edge : incoming[node]) {
            const double candidate = safe_add(edge.cost, dist);
            if (better_shortest_path_choice(
                    candidate, edge, result.value[edge.from],
                    result.policy[edge.from], best_next[edge.from])) {
                result.value[edge.from] = candidate;
                result.policy[edge.from] = edge.action;
                best_next[edge.from] = edge.to;
                pq.push({candidate, edge.from});
            }
        }
    }

    PolicyCheckResult check = check_policy_proper(graph, result.policy);
    if (check.proper) {
        result.value = evaluate_proper_policy_dag(graph, result.policy);
        result.success = true;
    }
    return result;
}

RolloutResult adversarial_rollout(
    const RobustGraph& graph,
    const std::vector<int>& policy,
    const std::vector<double>& J,
    int start,
    int max_steps
) {
    graph.validate();
    RolloutResult result;
    if (policy.size() != static_cast<size_t>(graph.n)) {
        return result;
    }
    if (J.size() != static_cast<size_t>(graph.n)) {
        return result;
    }
    if (start < 0 || start >= graph.n) {
        return result;
    }
    int x = start;
    result.path.push_back(x);

    for (int step = 0; step < max_steps; ++step) {
        if (graph.is_terminal(x)) {
            result.terminated = true;
            result.steps = step;
            return result;
        }

        const int a = policy[x];
        if (a < 0 || a >= static_cast<int>(graph.nodes[x].actions.size())) {
            result.steps = step;
            return result;
        }

        double worst = -INF;
        Transition chosen;
        for (const auto& tr : graph.nodes[x].actions[a].trans) {
            const double next_value = graph.is_terminal(tr.to) ? 0.0 : J[tr.to];
            const double score = safe_add(tr.cost, next_value);
            if (score > worst + EPS ||
                (std::abs(score - worst) <= EPS && (chosen.to < 0 || tr.to < chosen.to))) {
                worst = score;
                chosen = tr;
            }
        }

        result.cost += chosen.cost;
        x = chosen.to;
        result.path.push_back(x);
    }

    result.steps = max_steps;
    result.terminated = graph.is_terminal(x);
    return result;
}

}  // namespace rsp
