#pragma once

#include "rsp/graph.hpp"

#include <vector>

namespace rsp {

struct DijkstraLikeResult {
    std::vector<double> value;
    std::vector<int> policy;
    bool success = false;
    int finalized_count = 0;
    std::vector<int> finalize_order;
};

DijkstraLikeResult dijkstra_like(const RobustGraph& graph);

}  // namespace rsp

