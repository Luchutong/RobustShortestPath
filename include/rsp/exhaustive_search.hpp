#pragma once

#include "rsp/graph.hpp"

#include <vector>

namespace rsp {

struct ExhaustiveSearchResult {
    std::vector<double> optimal_value_by_state;
    std::vector<int> best_policy_for_start;
    bool success = false;
    int checked_policies = 0;
    int proper_policies = 0;
};

ExhaustiveSearchResult exhaustive_search(const RobustGraph& graph);

}  // namespace rsp
