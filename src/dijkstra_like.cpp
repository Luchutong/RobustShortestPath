#include "rsp/dijkstra_like.hpp"

#include "rsp/bellman.hpp"
#include "rsp/utils.hpp"

#include <cmath>

namespace rsp {
namespace {

bool better_candidate(double value, int node, int action,
                      double best_value, int best_node, int best_action) {
    if (less_with_eps(value, best_value)) {
        return true;
    }
    if (std::abs(value - best_value) > EPS) {
        return false;
    }
    if (best_node < 0 || node < best_node) {
        return true;
    }
    return node == best_node && action < best_action;
}

}  // namespace

DijkstraLikeResult dijkstra_like(const RobustGraph& graph) {
    graph.validate();
    DijkstraLikeResult result;
    result.value.assign(graph.n, INF);
    result.policy.assign(graph.n, -1);
    result.value[graph.terminal] = 0.0;

    std::vector<char> finalized(graph.n, false);
    finalized[graph.terminal] = true;
    result.finalize_order.push_back(graph.terminal);
    result.finalized_count = 1;

    // Negative costs are already rejected by graph.validate() above, so no
    // extra guard is needed here.
    while (result.finalized_count < graph.n) {
        int best_node = -1;
        int best_action = -1;
        double best_value = INF;

        for (int x = 0; x < graph.n; ++x) {
            if (finalized[x]) {
                continue;
            }
            for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
                bool all_successors_finalized = true;
                for (const auto& tr : graph.nodes[x].actions[a].trans) {
                    if (!finalized[tr.to]) {
                        all_successors_finalized = false;
                        break;
                    }
                }
                if (!all_successors_finalized) {
                    continue;
                }
                const double candidate = action_value(graph, x, a, result.value);
                if (better_candidate(candidate, x, a, best_value, best_node, best_action)) {
                    best_value = candidate;
                    best_node = x;
                    best_action = a;
                }
            }
        }

        if (best_node < 0) {
            result.success = false;
            return result;
        }

        finalized[best_node] = true;
        result.value[best_node] = best_value;
        result.policy[best_node] = best_action;
        result.finalize_order.push_back(best_node);
        ++result.finalized_count;
    }

    result.success = true;
    return result;
}

}  // namespace rsp
