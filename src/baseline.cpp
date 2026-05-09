#include "rsp/baseline.hpp"

#include "rsp/bellman.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

namespace rsp {

BaselineResult deterministic_dijkstra_baseline(
    const RobustGraph& graph,
    DeterministicMode mode
) {
    graph.validate();
    BaselineResult result;
    result.policy.assign(graph.n, -1);
    result.value.assign(graph.n, INF);

    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }

        double best_score = INF;
        int best_action = 0;
        for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
            const auto& action = graph.nodes[x].actions[a];
            double score = INF;
            if (mode == DeterministicMode::Nominal) {
                score = action.trans.front().cost;
            } else if (mode == DeterministicMode::BestCase) {
                for (const auto& tr : action.trans) {
                    if (less_with_eps(tr.cost, score)) {
                        score = tr.cost;
                    }
                }
            } else {
                score = -INF;
                for (const auto& tr : action.trans) {
                    if (tr.cost > score) {
                        score = tr.cost;
                    }
                }
            }
            if (less_with_eps(score, best_score)) {
                best_score = score;
                best_action = a;
            }
        }
        result.policy[x] = best_action;
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
            if (score > worst) {
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

