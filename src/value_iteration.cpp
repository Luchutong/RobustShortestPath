#include "rsp/value_iteration.hpp"

#include "rsp/bellman.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <algorithm>

namespace rsp {

ValueIterationResult value_iteration(
    const RobustGraph& graph,
    double epsilon,
    int max_iter,
    bool init_with_inf,
    bool save_history
) {
    graph.validate();
    ValueIterationResult result;
    result.value.assign(graph.n, init_with_inf ? INF : 0.0);
    result.value[graph.terminal] = 0.0;

    if (save_history) {
        result.value_history.push_back(result.value);
    }

    for (int iter = 1; iter <= max_iter; ++iter) {
        std::vector<double> next = bellman_update(graph, result.value);
        double residual = 0.0;
        for (int x = 0; x < graph.n; ++x) {
            residual = std::max(residual, finite_abs_diff(next[x], result.value[x]));
        }

        result.value = std::move(next);
        result.iterations = iter;
        result.residual_history.push_back(residual);
        if (save_history) {
            result.value_history.push_back(result.value);
        }

        if (!is_inf(residual) && residual < epsilon) {
            result.converged = true;
            break;
        }
    }

    result.policy = extract_greedy_policy(graph, result.value);
    result.final_policy_proper = check_policy_proper(graph, result.policy).proper;
    result.all_values_finite = true;
    for (int x = 0; x < graph.n; ++x) {
        if (!graph.is_terminal(x) && is_inf(result.value[x])) {
            result.all_values_finite = false;
            break;
        }
    }
    return result;
}

}  // namespace rsp
