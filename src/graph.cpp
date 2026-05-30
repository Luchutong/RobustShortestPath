#include "rsp/graph.hpp"

#include "rsp/utils.hpp"

#include <cmath>

namespace rsp {

int RobustGraph::total_actions() const {
    int total = 0;
    for (const auto& node : nodes) {
        total += static_cast<int>(node.actions.size());
    }
    return total;
}

int RobustGraph::total_transitions() const {
    int total = 0;
    for (const auto& node : nodes) {
        for (const auto& action : node.actions) {
            total += static_cast<int>(action.trans.size());
        }
    }
    return total;
}

void RobustGraph::validate() const {
    if (n <= 0) {
        throw std::invalid_argument("graph must contain at least one node");
    }
    if (terminal < 0 || terminal >= n) {
        throw std::invalid_argument("terminal is out of range");
    }
    if (static_cast<int>(nodes.size()) != n) {
        throw std::invalid_argument("nodes.size() must equal n");
    }

    for (int x = 0; x < n; ++x) {
        if (nodes[x].id != x) {
            throw std::invalid_argument("node id must match its index");
        }
        if (is_terminal(x)) {
            if (!nodes[x].actions.empty()) {
                throw std::invalid_argument("terminal node must not have actions");
            }
            continue;
        }
        if (nodes[x].actions.empty()) {
            throw std::invalid_argument("non-terminal node has no action");
        }
        for (const auto& action : nodes[x].actions) {
            if (action.trans.empty()) {
                throw std::invalid_argument("action has empty successor set");
            }
            for (const auto& tr : action.trans) {
                if (tr.to < 0 || tr.to >= n) {
                    throw std::invalid_argument("successor is out of range");
                }
                if (!std::isfinite(tr.cost)) {
                    throw std::invalid_argument("transition cost must be finite");
                }
                if (tr.cost < 0.0) {
                    throw std::invalid_argument("transition cost must be non-negative");
                }
                if (tr.cost >= INF / 2.0) {
                    throw std::invalid_argument(
                        "transition cost too large (collides with INF sentinel)");
                }
            }
        }
        // action_id is the label reported in policies.csv; require it to be
        // unique within a node so the reported policy is unambiguous.
        const auto& acts = nodes[x].actions;
        for (std::size_t i = 0; i < acts.size(); ++i) {
            for (std::size_t j = i + 1; j < acts.size(); ++j) {
                if (acts[i].action_id == acts[j].action_id) {
                    throw std::invalid_argument("duplicate action_id within a node");
                }
            }
        }
    }
}

}  // namespace rsp
