#pragma once

#include "rsp/graph.hpp"

#include <string>
#include <vector>

namespace rsp {

enum class AlgorithmKind {
    Core,
    Baseline
};

struct AlgorithmOptions {
    double epsilon = 1e-9;
    int max_iter = 100000;
    bool init_with_inf = true;
    bool save_history = true;
};

struct AlgorithmRunResult {
    std::string name;
    AlgorithmKind kind = AlgorithmKind::Core;
    std::vector<double> value;
    std::vector<int> policy;
    std::vector<double> residual_history;
    int iterations = 0;
    bool converged = false;
    bool success = false;
};

std::vector<std::string> core_algorithm_names();
std::vector<std::string> baseline_algorithm_names();
std::vector<std::string> all_algorithm_names();

AlgorithmRunResult run_algorithm(
    const RobustGraph& graph,
    const std::string& algorithm_name,
    const AlgorithmOptions& options = {}
);

}  // namespace rsp

