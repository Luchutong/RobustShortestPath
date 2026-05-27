#include "rsp/policy_iteration.hpp"

#include "rsp/bellman.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <algorithm>
#include <stdexcept>

namespace rsp {

PolicyIterationResult policy_iteration(
    const RobustGraph& graph,
    int max_outer_iter,
    double epsilon
) {
    graph.validate();
    PolicyIterationResult result;

    ProperPolicyResult init = find_initial_proper_policy(graph);
    if (!init.exists) {
        throw std::runtime_error("no initial proper policy exists");
    }

    result.policy = init.policy;
    std::vector<double> previous_value(graph.n, INF);

    for (int iter = 1; iter <= max_outer_iter; ++iter) {
        std::vector<double> J = evaluate_proper_policy_dag(graph, result.policy);
        double residual = 0.0;
        for (int x = 0; x < graph.n; ++x) {
            residual = std::max(residual, finite_abs_diff(J[x], previous_value[x]));
        }
        previous_value = J;

        std::vector<int> next_policy = result.policy;
        int changes = 0;

        for (int x = 0; x < graph.n; ++x) {
            if (graph.is_terminal(x)) {
                continue;
            }
            const int old_action = result.policy[x];
            double best = action_value(graph, x, old_action, J);
            int best_action = old_action;

            for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
                const double candidate = action_value(graph, x, a, J);
                if (less_with_eps(candidate, best)) {
                    best = candidate;
                    best_action = a;
                }
            }

            if (best_action != old_action) {
                next_policy[x] = best_action;
                ++changes;
            }
        }

        result.outer_iterations = iter;
        result.policy_change_history.push_back(changes);
        result.residual_history.push_back(residual);

        if (changes == 0) {
            result.value = J;
            result.converged = true;
            break;
        }

        PolicyCheckResult check = check_policy_proper(graph, next_policy);
        if (!check.proper) {
            result.value = J;
            result.final_policy_proper = false;
            throw std::runtime_error("policy improvement produced an improper policy");
        }
        result.policy = std::move(next_policy);
    }

    if (result.value.empty()) {
        result.value = evaluate_proper_policy_dag(graph, result.policy);
    }
    result.final_policy_proper = check_policy_proper(graph, result.policy).proper;
    return result;
}

}  // namespace rsp
