#pragma once

#include "rsp/graph.hpp"

#include <vector>

namespace rsp {

struct PolicyIterationResult {
    std::vector<double> value;
    std::vector<int> policy;
    int outer_iterations = 0;
    bool converged = false;
    bool final_policy_proper = false;
    std::vector<int> policy_change_history;
    std::vector<double> residual_history;
};

PolicyIterationResult policy_iteration(
    const RobustGraph& graph,
    int max_outer_iter = 10000,
    double epsilon = 1e-9
);

}  // namespace rsp

