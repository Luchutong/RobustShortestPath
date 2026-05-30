#include "rsp/exhaustive_search.hpp"

#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <functional>

namespace rsp {

ExhaustiveSearchResult exhaustive_search(const RobustGraph& graph) {
    graph.validate();
    ExhaustiveSearchResult result;
    result.optimal_value_by_state.assign(graph.n, INF);
    result.best_policy_for_start.assign(graph.n, -1);

    // Exhaustive enumeration is exponential (it tries the product of all
    // non-terminal action counts) and recurses to depth n. Guard against
    // unbounded hangs and stack overflow by refusing graphs that are too
    // large, returning success=false to match the graceful-failure contract
    // used by value_iteration / dijkstra_like. exhaustive is a small-graph
    // ground-truth oracle, not a general solver.
    constexpr long long kMaxPolicies = 5'000'000;
    constexpr int kMaxNodes = 2000;  // recursion depth == n
    if (graph.n > kMaxNodes) {
        return result;  // success stays false
    }
    long long policy_count = 1;
    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }
        const long long actions = static_cast<long long>(graph.nodes[x].actions.size());
        if (actions <= 0 || policy_count > kMaxPolicies / actions) {
            return result;  // too large -> success=false (no overflow)
        }
        policy_count *= actions;
    }

    std::vector<int> current(graph.n, -1);

    std::function<void(int)> dfs = [&](int x) {
        if (x == graph.n) {
            ++result.checked_policies;
            PolicyCheckResult check = check_policy_proper(graph, current);
            if (!check.proper) {
                return;
            }
            ++result.proper_policies;
            std::vector<double> value = evaluate_proper_policy_dag(graph, current);
            bool improved_start = less_with_eps(value[0], result.optimal_value_by_state[0]);
            for (int i = 0; i < graph.n; ++i) {
                if (less_with_eps(value[i], result.optimal_value_by_state[i])) {
                    result.optimal_value_by_state[i] = value[i];
                }
            }
            if (improved_start || result.best_policy_for_start[0] < 0) {
                result.best_policy_for_start = current;
            }
            result.success = true;
            return;
        }

        if (graph.is_terminal(x)) {
            current[x] = -1;
            dfs(x + 1);
            return;
        }

        for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
            current[x] = a;
            dfs(x + 1);
        }
    };

    dfs(0);
    return result;
}

}  // namespace rsp
