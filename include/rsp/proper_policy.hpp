#pragma once

#include "rsp/graph.hpp"

#include <vector>

namespace rsp {

struct ProperPolicyResult {
    bool exists = false;
    std::vector<int> policy;
    std::vector<int> rank;
};

struct PolicyCheckResult {
    bool proper = false;
    bool has_cycle = false;
    bool all_reach_terminal = false;
    std::vector<int> topo_order;
};

ProperPolicyResult find_initial_proper_policy(const RobustGraph& graph);

PolicyCheckResult check_policy_proper(
    const RobustGraph& graph,
    const std::vector<int>& policy
);

std::vector<double> evaluate_proper_policy_dag(
    const RobustGraph& graph,
    const std::vector<int>& policy
);

}  // namespace rsp

