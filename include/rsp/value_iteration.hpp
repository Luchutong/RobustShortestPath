#pragma once

#include "rsp/graph.hpp"

#include <vector>

namespace rsp {

struct ValueIterationResult {
    std::vector<double> value;
    std::vector<int> policy;
    int iterations = 0;
    bool converged = false;
    std::vector<double> residual_history;
    std::vector<std::vector<double>> value_history;
};

ValueIterationResult value_iteration(
    const RobustGraph& graph,
    double epsilon = 1e-9,
    int max_iter = 100000,
    bool init_with_inf = true,
    bool save_history = true
);

}  // namespace rsp

