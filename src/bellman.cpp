#include "rsp/bellman.hpp"

#include "rsp/utils.hpp"

#include <stdexcept>

namespace rsp {

double action_value(
    const RobustGraph& graph,
    int x,
    int action_index,
    const std::vector<double>& J
) {
    if (x < 0 || x >= graph.n) {
        throw std::out_of_range("node index out of range");
    }
    if (static_cast<int>(J.size()) != graph.n) {
        throw std::invalid_argument("value vector size does not match graph.n");
    }
    if (graph.is_terminal(x)) {
        return 0.0;
    }
    const auto& actions = graph.nodes[x].actions;
    if (action_index < 0 || action_index >= static_cast<int>(actions.size())) {
        throw std::out_of_range("action index out of range");
    }

    double worst = -INF;
    for (const auto& tr : actions[action_index].trans) {
        const double next = graph.is_terminal(tr.to) ? 0.0 : J[tr.to];
        const double candidate = safe_add(tr.cost, next);
        if (candidate > worst) {
            worst = candidate;
        }
    }
    return worst;
}

int greedy_action(
    const RobustGraph& graph,
    int x,
    const std::vector<double>& J
) {
    if (graph.is_terminal(x)) {
        return -1;
    }

    double best = INF;
    int best_idx = -1;
    const auto& actions = graph.nodes[x].actions;
    for (int i = 0; i < static_cast<int>(actions.size()); ++i) {
        const double candidate = action_value(graph, x, i, J);
        if (less_with_eps(candidate, best)) {
            best = candidate;
            best_idx = i;
        }
    }
    return best_idx;
}

std::vector<int> extract_greedy_policy(
    const RobustGraph& graph,
    const std::vector<double>& J
) {
    std::vector<int> policy(graph.n, -1);
    for (int x = 0; x < graph.n; ++x) {
        policy[x] = greedy_action(graph, x, J);
    }
    return policy;
}

std::vector<double> bellman_update(
    const RobustGraph& graph,
    const std::vector<double>& J
) {
    std::vector<double> next(graph.n, INF);
    next[graph.terminal] = 0.0;
    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }
        // Single pass over actions: keep the minimum action value directly so
        // we do not recompute action_value for the chosen action a second time.
        double best = INF;
        const auto& actions = graph.nodes[x].actions;
        for (int a = 0; a < static_cast<int>(actions.size()); ++a) {
            const double candidate = action_value(graph, x, a, J);
            if (less_with_eps(candidate, best)) {
                best = candidate;
            }
        }
        next[x] = best;
    }
    return next;
}

}  // namespace rsp

