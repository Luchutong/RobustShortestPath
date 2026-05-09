#pragma once

#include "rsp/graph.hpp"

#include <vector>

namespace rsp {

enum class DeterministicMode {
    Nominal,
    BestCase,
    WorstImmediate
};

struct BaselineResult {
    std::vector<double> value;
    std::vector<int> policy;
    bool success = false;
};

struct RolloutResult {
    std::vector<int> path;
    double cost = 0.0;
    bool terminated = false;
    int steps = 0;
};

BaselineResult deterministic_dijkstra_baseline(
    const RobustGraph& graph,
    DeterministicMode mode
);

RolloutResult adversarial_rollout(
    const RobustGraph& graph,
    const std::vector<int>& policy,
    const std::vector<double>& J,
    int start,
    int max_steps
);

}  // namespace rsp

