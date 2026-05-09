#pragma once

#include "rsp/dijkstra_like.hpp"
#include "rsp/exhaustive_search.hpp"
#include "rsp/graph.hpp"
#include "rsp/policy_iteration.hpp"
#include "rsp/value_iteration.hpp"

#include <string>
#include <vector>

namespace rsp {

RobustGraph read_graph_txt(const std::string& path);
void write_graph_json(const RobustGraph& graph, const std::string& path);

void append_values_csv(
    const std::string& path,
    const std::string& graph_id,
    const std::string& algorithm,
    const std::vector<double>& value
);

void append_policies_csv(
    const std::string& path,
    const std::string& graph_id,
    const std::string& algorithm,
    const RobustGraph& graph,
    const std::vector<int>& policy
);

void append_residual_history_csv(
    const std::string& path,
    const std::string& graph_id,
    const std::string& algorithm,
    const std::vector<double>& residual_history
);

void append_runtime_csv(
    const std::string& path,
    const std::string& graph_id,
    const RobustGraph& graph,
    const std::string& algorithm,
    double runtime_ms,
    int iterations,
    bool converged,
    const std::vector<double>& value,
    bool success
);

}  // namespace rsp

