#pragma once

#include "rsp/graph.hpp"

#include <vector>

namespace rsp {

double action_value(
    const RobustGraph& graph,
    int x,
    int action_index,
    const std::vector<double>& J
);

int greedy_action(
    const RobustGraph& graph,
    int x,
    const std::vector<double>& J
);

std::vector<int> extract_greedy_policy(
    const RobustGraph& graph,
    const std::vector<double>& J
);

std::vector<double> bellman_update(
    const RobustGraph& graph,
    const std::vector<double>& J
);

}  // namespace rsp

