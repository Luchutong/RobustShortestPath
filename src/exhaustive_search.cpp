#include "rsp/exhaustive_search.hpp"

#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <functional>

namespace rsp {

ExhaustiveSearchResult exhaustive_search(const RobustGraph& graph) {
    graph.validate();
    ExhaustiveSearchResult result;
    result.value.assign(graph.n, INF);
    result.policy.assign(graph.n, -1);

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
            bool improved_start = less_with_eps(value[0], result.value[0]);
            for (int i = 0; i < graph.n; ++i) {
                if (less_with_eps(value[i], result.value[i])) {
                    result.value[i] = value[i];
                }
            }
            if (improved_start || result.policy[0] < 0) {
                result.policy = current;
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

