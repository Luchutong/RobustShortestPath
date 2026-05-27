#include "rsp/runner.hpp"

#include "rsp/baseline.hpp"
#include "rsp/dijkstra_like.hpp"
#include "rsp/exhaustive_search.hpp"
#include "rsp/policy_iteration.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"
#include "rsp/value_iteration.hpp"

#include <stdexcept>
#include <utility>

namespace rsp {

std::vector<std::string> core_algorithm_names() {
    return {"vi", "pi", "dijkstra"};
}

std::vector<std::string> baseline_algorithm_names() {
    return {"exhaustive", "baseline_nominal", "baseline_bestcase", "baseline_worst_immediate"};
}

std::vector<std::string> all_algorithm_names() {
    std::vector<std::string> names = core_algorithm_names();
    const auto baselines = baseline_algorithm_names();
    names.insert(names.end(), baselines.begin(), baselines.end());
    return names;
}

AlgorithmRunResult run_algorithm(
    const RobustGraph& graph,
    const std::string& algorithm_name,
    const AlgorithmOptions& options
) {
    AlgorithmRunResult out;
    out.name = algorithm_name;

    if (algorithm_name == "vi") {
        auto result = value_iteration(
            graph,
            options.epsilon,
            options.max_iter,
            options.init_with_inf,
            options.save_history
        );
        out.value = std::move(result.value);
        out.policy = std::move(result.policy);
        out.residual_history = std::move(result.residual_history);
        out.iterations = result.iterations;
        out.converged = result.converged;
        out.success = result.converged &&
            result.final_policy_proper &&
            result.all_values_finite;
        return out;
    }

    if (algorithm_name == "pi") {
        auto result = policy_iteration(graph, options.max_iter, options.epsilon);
        out.value = std::move(result.value);
        out.policy = std::move(result.policy);
        out.residual_history = std::move(result.residual_history);
        out.iterations = result.outer_iterations;
        out.converged = result.converged;
        out.success = result.converged && result.final_policy_proper;
        return out;
    }

    if (algorithm_name == "dijkstra") {
        auto result = dijkstra_like(graph);
        out.value = std::move(result.value);
        out.policy = std::move(result.policy);
        out.iterations = result.finalized_count;
        out.converged = result.success;
        out.success = result.success;
        return out;
    }

    if (algorithm_name == "exhaustive") {
        auto result = exhaustive_search(graph);
        out.kind = AlgorithmKind::Baseline;
        out.value = std::move(result.value);
        out.policy = std::move(result.policy);
        out.iterations = result.checked_policies;
        out.converged = result.success;
        out.success = result.success;
        return out;
    }

    DeterministicMode mode = DeterministicMode::Nominal;
    bool is_baseline = true;
    if (algorithm_name == "baseline_nominal") {
        mode = DeterministicMode::Nominal;
    } else if (algorithm_name == "baseline_bestcase") {
        mode = DeterministicMode::BestCase;
    } else if (algorithm_name == "baseline_worst_immediate") {
        mode = DeterministicMode::WorstImmediate;
    } else {
        is_baseline = false;
    }

    if (is_baseline) {
        auto result = deterministic_dijkstra_baseline(graph, mode);
        out.kind = AlgorithmKind::Baseline;
        out.value = std::move(result.value);
        out.policy = std::move(result.policy);
        out.iterations = 0;
        out.converged = result.success;
        out.success = result.success;
        return out;
    }

    throw std::invalid_argument("unknown algorithm: " + algorithm_name);
}

}  // namespace rsp
