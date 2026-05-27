#include "rsp/proper_policy.hpp"

#include "rsp/bellman.hpp"
#include "rsp/utils.hpp"

#include <queue>
#include <stdexcept>

namespace rsp {

ProperPolicyResult find_initial_proper_policy(const RobustGraph& graph) {
    graph.validate();
    ProperPolicyResult result;
    result.policy.assign(graph.n, -1);
    result.rank.assign(graph.n, -1);

    std::vector<char> known(graph.n, false);
    known[graph.terminal] = true;
    result.rank[graph.terminal] = 0;

    int known_count = 1;
    int rank = 1;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int x = 0; x < graph.n; ++x) {
            if (known[x]) {
                continue;
            }
            const auto& actions = graph.nodes[x].actions;
            for (int a = 0; a < static_cast<int>(actions.size()); ++a) {
                bool all_known = true;
                for (const auto& tr : actions[a].trans) {
                    if (!known[tr.to]) {
                        all_known = false;
                        break;
                    }
                }
                if (all_known) {
                    known[x] = true;
                    result.policy[x] = a;
                    result.rank[x] = rank;
                    ++known_count;
                    changed = true;
                    break;
                }
            }
        }
        ++rank;
    }

    result.exists = known_count == graph.n;
    return result;
}

PolicyCheckResult check_policy_proper(
    const RobustGraph& graph,
    const std::vector<int>& policy
) {
    graph.validate();
    if (static_cast<int>(policy.size()) != graph.n) {
        throw std::invalid_argument("policy size does not match graph.n");
    }

    PolicyCheckResult result;
    if (policy[graph.terminal] != -1) {
        return result;
    }
    std::vector<std::vector<int>> adj(graph.n);
    std::vector<std::vector<int>> rev(graph.n);
    std::vector<int> indeg(graph.n, 0);

    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }
        const int a = policy[x];
        if (a < 0 || a >= static_cast<int>(graph.nodes[x].actions.size())) {
            result.has_cycle = true;
            return result;
        }
        for (const auto& tr : graph.nodes[x].actions[a].trans) {
            rev[tr.to].push_back(x);
            if (!graph.is_terminal(tr.to)) {
                adj[x].push_back(tr.to);
                ++indeg[tr.to];
            }
        }
    }

    std::queue<int> q;
    for (int x = 0; x < graph.n; ++x) {
        if (!graph.is_terminal(x) && indeg[x] == 0) {
            q.push(x);
        }
    }

    while (!q.empty()) {
        const int x = q.front();
        q.pop();
        result.topo_order.push_back(x);
        for (int y : adj[x]) {
            --indeg[y];
            if (indeg[y] == 0) {
                q.push(y);
            }
        }
    }

    result.has_cycle = static_cast<int>(result.topo_order.size()) != graph.n - 1;

    std::vector<char> can_reach_terminal(graph.n, false);
    std::queue<int> rq;
    can_reach_terminal[graph.terminal] = true;
    rq.push(graph.terminal);
    while (!rq.empty()) {
        const int y = rq.front();
        rq.pop();
        for (int x : rev[y]) {
            if (!can_reach_terminal[x]) {
                can_reach_terminal[x] = true;
                rq.push(x);
            }
        }
    }

    result.all_reach_terminal = true;
    for (int x = 0; x < graph.n; ++x) {
        if (!can_reach_terminal[x]) {
            result.all_reach_terminal = false;
            break;
        }
    }
    result.proper = !result.has_cycle && result.all_reach_terminal;
    return result;
}

std::vector<double> evaluate_proper_policy_dag(
    const RobustGraph& graph,
    const std::vector<int>& policy
) {
    PolicyCheckResult check = check_policy_proper(graph, policy);
    if (!check.proper) {
        throw std::invalid_argument("cannot evaluate improper policy as DAG");
    }

    std::vector<double> J(graph.n, INF);
    J[graph.terminal] = 0.0;
    for (auto it = check.topo_order.rbegin(); it != check.topo_order.rend(); ++it) {
        const int x = *it;
        J[x] = action_value(graph, x, policy[x], J);
    }
    return J;
}

}  // namespace rsp
