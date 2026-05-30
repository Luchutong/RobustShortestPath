# 附录 F　核心源码完整清单

> 本附录给出 `red` 分支九个核心源文件的**逐字完整清单**(与正文第四部分「源码精讲」的片段一一对应,此处为完整上下文,便于查证)。每段开头标注文件路径。

## F.1　`include/rsp/utils.hpp`

> 数值工具:INF 哨兵、is_inf/safe_add/finite_abs_diff

```cpp
// === include/rsp/utils.hpp ===
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace rsp {

constexpr double INF = 1e100;
constexpr double EPS = 1e-9;

inline bool is_inf(double x) {
    return x >= INF / 2.0;
}

inline double safe_add(double a, double b) {
    if (is_inf(a) || is_inf(b)) return INF;
    if (a > INF / 2.0 - b) return INF;
    return a + b;
}

inline double finite_abs_diff(double a, double b) {
    if (is_inf(a) && is_inf(b)) return 0.0;
    if (is_inf(a) || is_inf(b)) return INF;
    return std::abs(a - b);
}

inline bool less_with_eps(double a, double b) {
    return a < b - EPS;
}

inline std::string format_value(double x) {
    return is_inf(x) ? "inf" : std::to_string(x);
}

}  // namespace rsp
```

## F.2　`include/rsp/graph.hpp`

> 核心数据结构:Transition/Action/Node/RobustGraph

```cpp
// === include/rsp/graph.hpp ===
#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace rsp {

struct Transition {
    int to = -1;
    double cost = 0.0;
};

struct Action {
    int action_id = -1;
    std::string name;
    std::vector<Transition> trans;
};

struct Node {
    int id = -1;
    std::vector<Action> actions;
};

class RobustGraph {
public:
    int n = 0;
    int terminal = -1;
    std::vector<Node> nodes;

    bool is_terminal(int x) const {
        return x == terminal;
    }

    int total_actions() const;
    int total_transitions() const;
    void validate() const;
};

}  // namespace rsp
```

## F.3　`src/bellman.cpp`

> Bellman 算子:action_value=H、greedy_action、bellman_update=T J

```cpp
// === src/bellman.cpp ===
#include "rsp/bellman.hpp"

#include "rsp/utils.hpp"

#include <stdexcept>

namespace rsp {

double action_value(
    const RobustGraph& graph,
    int x,
    int action_index,
    const std::vector<double>& J
) {
    if (x < 0 || x >= graph.n) {
        throw std::out_of_range("node index out of range");
    }
    if (static_cast<int>(J.size()) != graph.n) {
        throw std::invalid_argument("value vector size does not match graph.n");
    }
    if (graph.is_terminal(x)) {
        return 0.0;
    }
    const auto& actions = graph.nodes[x].actions;
    if (action_index < 0 || action_index >= static_cast<int>(actions.size())) {
        throw std::out_of_range("action index out of range");
    }

    double worst = -INF;
    for (const auto& tr : actions[action_index].trans) {
        const double next = graph.is_terminal(tr.to) ? 0.0 : J[tr.to];
        const double candidate = safe_add(tr.cost, next);
        if (candidate > worst) {
            worst = candidate;
        }
    }
    return worst;
}

int greedy_action(
    const RobustGraph& graph,
    int x,
    const std::vector<double>& J
) {
    if (graph.is_terminal(x)) {
        return -1;
    }

    double best = INF;
    int best_idx = -1;
    const auto& actions = graph.nodes[x].actions;
    for (int i = 0; i < static_cast<int>(actions.size()); ++i) {
        const double candidate = action_value(graph, x, i, J);
        if (less_with_eps(candidate, best)) {
            best = candidate;
            best_idx = i;
        }
    }
    return best_idx;
}

std::vector<int> extract_greedy_policy(
    const RobustGraph& graph,
    const std::vector<double>& J
) {
    std::vector<int> policy(graph.n, -1);
    for (int x = 0; x < graph.n; ++x) {
        policy[x] = greedy_action(graph, x, J);
    }
    return policy;
}

std::vector<double> bellman_update(
    const RobustGraph& graph,
    const std::vector<double>& J
) {
    std::vector<double> next(graph.n, INF);
    next[graph.terminal] = 0.0;
    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }
        // Single pass over actions: keep the minimum action value directly so
        // we do not recompute action_value for the chosen action a second time.
        double best = INF;
        const auto& actions = graph.nodes[x].actions;
        for (int a = 0; a < static_cast<int>(actions.size()); ++a) {
            const double candidate = action_value(graph, x, a, J);
            if (less_with_eps(candidate, best)) {
                best = candidate;
            }
        }
        next[x] = best;
    }
    return next;
}

}  // namespace rsp
```

## F.4　`src/proper_policy.cpp`

> proper 策略:初始分层、Kahn 判环、DAG 最长路求值

```cpp
// === src/proper_policy.cpp ===
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
```

## F.5　`src/value_iteration.cpp`

> 值迭代 VI:T^k J→J*,残差与有限终止

```cpp
// === src/value_iteration.cpp ===
#include "rsp/value_iteration.hpp"

#include "rsp/bellman.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <algorithm>

namespace rsp {

ValueIterationResult value_iteration(
    const RobustGraph& graph,
    double epsilon,
    int max_iter,
    bool init_with_inf,
    bool save_history
) {
    graph.validate();
    ValueIterationResult result;
    result.value.assign(graph.n, init_with_inf ? INF : 0.0);
    result.value[graph.terminal] = 0.0;

    if (save_history) {
        result.value_history.push_back(result.value);
    }

    for (int iter = 1; iter <= max_iter; ++iter) {
        std::vector<double> next = bellman_update(graph, result.value);
        double residual = 0.0;
        for (int x = 0; x < graph.n; ++x) {
            residual = std::max(residual, finite_abs_diff(next[x], result.value[x]));
        }

        result.value = std::move(next);
        result.iterations = iter;
        result.residual_history.push_back(residual);
        if (save_history) {
            result.value_history.push_back(result.value);
        }

        if (!is_inf(residual) && residual < epsilon) {
            result.converged = true;
            break;
        }
    }

    result.policy = extract_greedy_policy(graph, result.value);
    result.final_policy_proper = check_policy_proper(graph, result.policy).proper;
    result.all_values_finite = true;
    for (int x = 0; x < graph.n; ++x) {
        if (!graph.is_terminal(x) && is_inf(result.value[x])) {
            result.all_values_finite = false;
            break;
        }
    }
    return result;
}

}  // namespace rsp
```

## F.6　`src/policy_iteration.cpp`

> 策略迭代 PI:DAG 求值 + 单调改进 + 优雅失败

```cpp
// === src/policy_iteration.cpp ===
#include "rsp/policy_iteration.hpp"

#include "rsp/bellman.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <algorithm>
#include <stdexcept>

namespace rsp {

PolicyIterationResult policy_iteration(
    const RobustGraph& graph,
    int max_outer_iter
) {
    graph.validate();
    PolicyIterationResult result;

    ProperPolicyResult init = find_initial_proper_policy(graph);
    if (!init.exists) {
        // No proper policy exists for this graph (some state cannot reach the
        // terminal against the worst-case successor). Fail gracefully and
        // consistently with value_iteration / dijkstra_like instead of
        // throwing, so batch experiments can record success=false and move on.
        result.value.assign(graph.n, INF);
        result.value[graph.terminal] = 0.0;
        result.policy = init.policy;
        result.converged = false;
        result.final_policy_proper = false;
        return result;
    }

    result.policy = init.policy;
    std::vector<double> previous_value(graph.n, INF);

    for (int iter = 1; iter <= max_outer_iter; ++iter) {
        std::vector<double> J = evaluate_proper_policy_dag(graph, result.policy);
        double residual = 0.0;
        for (int x = 0; x < graph.n; ++x) {
            residual = std::max(residual, finite_abs_diff(J[x], previous_value[x]));
        }
        previous_value = J;

        std::vector<int> next_policy = result.policy;
        int changes = 0;

        for (int x = 0; x < graph.n; ++x) {
            if (graph.is_terminal(x)) {
                continue;
            }
            const int old_action = result.policy[x];
            double best = action_value(graph, x, old_action, J);
            int best_action = old_action;

            for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
                const double candidate = action_value(graph, x, a, J);
                if (less_with_eps(candidate, best)) {
                    best = candidate;
                    best_action = a;
                }
            }

            if (best_action != old_action) {
                next_policy[x] = best_action;
                ++changes;
            }
        }

        result.outer_iterations = iter;
        result.policy_change_history.push_back(changes);
        result.residual_history.push_back(residual);

        if (changes == 0) {
            result.value = J;
            result.converged = true;
            break;
        }

        PolicyCheckResult check = check_policy_proper(graph, next_policy);
        if (!check.proper) {
            // For a robust SSP, strictly-improving policy iteration over a
            // proper policy is guaranteed to stay proper, so reaching here
            // would signal a numerical edge case. Fail gracefully (report the
            // last proper evaluation and converged=false) rather than throwing.
            result.value = J;
            result.converged = false;
            result.final_policy_proper = false;
            return result;
        }
        result.policy = std::move(next_policy);
    }

    if (result.value.empty()) {
        result.value = evaluate_proper_policy_dag(graph, result.policy);
    }
    result.final_policy_proper = check_policy_proper(graph, result.policy).proper;
    return result;
}

}  // namespace rsp
```

## F.7　`src/dijkstra_like.cpp`

> Dijkstra-like:永久标号、N+1 终止

```cpp
// === src/dijkstra_like.cpp ===
#include "rsp/dijkstra_like.hpp"

#include "rsp/bellman.hpp"
#include "rsp/utils.hpp"

#include <cmath>

namespace rsp {
namespace {

bool better_candidate(double value, int node, int action,
                      double best_value, int best_node, int best_action) {
    if (less_with_eps(value, best_value)) {
        return true;
    }
    if (std::abs(value - best_value) > EPS) {
        return false;
    }
    if (best_node < 0 || node < best_node) {
        return true;
    }
    return node == best_node && action < best_action;
}

}  // namespace

DijkstraLikeResult dijkstra_like(const RobustGraph& graph) {
    graph.validate();
    DijkstraLikeResult result;
    result.value.assign(graph.n, INF);
    result.policy.assign(graph.n, -1);
    result.value[graph.terminal] = 0.0;

    std::vector<char> finalized(graph.n, false);
    finalized[graph.terminal] = true;
    result.finalize_order.push_back(graph.terminal);
    result.finalized_count = 1;

    // Negative costs are already rejected by graph.validate() above, so no
    // extra guard is needed here.
    while (result.finalized_count < graph.n) {
        int best_node = -1;
        int best_action = -1;
        double best_value = INF;

        for (int x = 0; x < graph.n; ++x) {
            if (finalized[x]) {
                continue;
            }
            for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
                bool all_successors_finalized = true;
                for (const auto& tr : graph.nodes[x].actions[a].trans) {
                    if (!finalized[tr.to]) {
                        all_successors_finalized = false;
                        break;
                    }
                }
                if (!all_successors_finalized) {
                    continue;
                }
                const double candidate = action_value(graph, x, a, result.value);
                if (better_candidate(candidate, x, a, best_value, best_node, best_action)) {
                    best_value = candidate;
                    best_node = x;
                    best_action = a;
                }
            }
        }

        if (best_node < 0) {
            result.success = false;
            return result;
        }

        finalized[best_node] = true;
        result.value[best_node] = best_value;
        result.policy[best_node] = best_action;
        result.finalize_order.push_back(best_node);
        ++result.finalized_count;
    }

    result.success = true;
    return result;
}

}  // namespace rsp
```

## F.8　`src/exhaustive_search.cpp`

> 穷举 ground-truth:枚举 proper 策略逐点取最小(含规模守卫)

```cpp
// === src/exhaustive_search.cpp ===
#include "rsp/exhaustive_search.hpp"

#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <functional>

namespace rsp {

ExhaustiveSearchResult exhaustive_search(const RobustGraph& graph) {
    graph.validate();
    ExhaustiveSearchResult result;
    result.optimal_value_by_state.assign(graph.n, INF);
    result.best_policy_for_start.assign(graph.n, -1);

    // Exhaustive enumeration is exponential (it tries the product of all
    // non-terminal action counts) and recurses to depth n. Guard against
    // unbounded hangs and stack overflow by refusing graphs that are too
    // large, returning success=false to match the graceful-failure contract
    // used by value_iteration / dijkstra_like. exhaustive is a small-graph
    // ground-truth oracle, not a general solver.
    constexpr long long kMaxPolicies = 5'000'000;
    constexpr int kMaxNodes = 2000;  // recursion depth == n
    if (graph.n > kMaxNodes) {
        return result;  // success stays false
    }
    long long policy_count = 1;
    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }
        const long long actions = static_cast<long long>(graph.nodes[x].actions.size());
        if (actions <= 0 || policy_count > kMaxPolicies / actions) {
            return result;  // too large -> success=false (no overflow)
        }
        policy_count *= actions;
    }

    std::vector<int> current(graph.n, -1);

    std::function<void(int)> dfs = [&](int x) {
        if (x == graph.n) {
            ++result.checked_policies;
            PolicyCheckResult check = check_policy_proper(graph, current);
            if (!check.proper) {
                return;
            }
            ++result.proper_policies;
            std::vector<double> value = evaluate_proper_policy_dag(graph, current);
            bool improved_start = less_with_eps(value[0], result.optimal_value_by_state[0]);
            for (int i = 0; i < graph.n; ++i) {
                if (less_with_eps(value[i], result.optimal_value_by_state[i])) {
                    result.optimal_value_by_state[i] = value[i];
                }
            }
            if (improved_start || result.best_policy_for_start[0] < 0) {
                result.best_policy_for_start = current;
            }
            result.success = true;
            return;
        }

        if (graph.is_terminal(x)) {
            current[x] = -1;
            dfs(x + 1);
            return;
        }

        for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
            current[x] = a;
            dfs(x + 1);
        }
    };

    dfs(0);
    return result;
}

}  // namespace rsp
```

## F.9　`src/baseline.cpp`

> 三种 deterministic baseline:nominal/bestcase/worst_immediate

```cpp
// === src/baseline.cpp ===
#include "rsp/baseline.hpp"

#include "rsp/bellman.hpp"
#include "rsp/proper_policy.hpp"
#include "rsp/utils.hpp"

#include <cmath>
#include <functional>
#include <queue>
#include <vector>

namespace rsp {
namespace {

struct DeterministicEdge {
    int from = -1;
    int to = -1;
    int action = -1;
    double cost = 0.0;
};

Transition worst_immediate_successor(const Action& action) {
    Transition chosen = action.trans.front();
    for (const auto& tr : action.trans) {
        if (tr.cost > chosen.cost + EPS ||
            (std::abs(tr.cost - chosen.cost) <= EPS && tr.to < chosen.to)) {
            chosen = tr;
        }
    }
    return chosen;
}

std::vector<DeterministicEdge> build_deterministic_edges(
    const RobustGraph& graph,
    DeterministicMode mode
) {
    std::vector<DeterministicEdge> edges;
    for (int x = 0; x < graph.n; ++x) {
        if (graph.is_terminal(x)) {
            continue;
        }
        for (int a = 0; a < static_cast<int>(graph.nodes[x].actions.size()); ++a) {
            const auto& action = graph.nodes[x].actions[a];
            if (mode == DeterministicMode::BestCase) {
                for (const auto& tr : action.trans) {
                    edges.push_back({x, tr.to, a, tr.cost});
                }
            } else if (mode == DeterministicMode::WorstImmediate) {
                const auto tr = worst_immediate_successor(action);
                edges.push_back({x, tr.to, a, tr.cost});
            } else {
                const auto& tr = action.trans.front();
                edges.push_back({x, tr.to, a, tr.cost});
            }
        }
    }
    return edges;
}

bool better_shortest_path_choice(
    double candidate,
    const DeterministicEdge& edge,
    double best_distance,
    int best_action,
    int best_next
) {
    if (less_with_eps(candidate, best_distance)) {
        return true;
    }
    if (std::abs(candidate - best_distance) > EPS) {
        return false;
    }
    if (best_action < 0 || edge.action < best_action) {
        return true;
    }
    return edge.action == best_action && edge.to < best_next;
}

}  // namespace

BaselineResult deterministic_dijkstra_baseline(
    const RobustGraph& graph,
    DeterministicMode mode
) {
    graph.validate();
    BaselineResult result;
    result.policy.assign(graph.n, -1);
    result.value.assign(graph.n, INF);
    // Negative costs are already rejected by graph.validate() above.

    std::vector<std::vector<DeterministicEdge>> incoming(graph.n);
    for (const auto& edge : build_deterministic_edges(graph, mode)) {
        incoming[edge.to].push_back(edge);
    }

    std::vector<int> best_next(graph.n, -1);
    using QueueItem = std::pair<double, int>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> pq;
    result.value[graph.terminal] = 0.0;
    pq.push({0.0, graph.terminal});

    while (!pq.empty()) {
        const auto [dist, node] = pq.top();
        pq.pop();
        if (std::abs(dist - result.value[node]) > EPS) {
            continue;
        }
        for (const auto& edge : incoming[node]) {
            const double candidate = safe_add(edge.cost, dist);
            if (better_shortest_path_choice(
                    candidate, edge, result.value[edge.from],
                    result.policy[edge.from], best_next[edge.from])) {
                result.value[edge.from] = candidate;
                result.policy[edge.from] = edge.action;
                best_next[edge.from] = edge.to;
                pq.push({candidate, edge.from});
            }
        }
    }

    PolicyCheckResult check = check_policy_proper(graph, result.policy);
    if (check.proper) {
        result.value = evaluate_proper_policy_dag(graph, result.policy);
        result.success = true;
    }
    return result;
}

RolloutResult adversarial_rollout(
    const RobustGraph& graph,
    const std::vector<int>& policy,
    const std::vector<double>& J,
    int start,
    int max_steps
) {
    graph.validate();
    RolloutResult result;
    if (policy.size() != static_cast<size_t>(graph.n)) {
        return result;
    }
    if (J.size() != static_cast<size_t>(graph.n)) {
        return result;
    }
    if (start < 0 || start >= graph.n) {
        return result;
    }
    int x = start;
    result.path.push_back(x);

    for (int step = 0; step < max_steps; ++step) {
        if (graph.is_terminal(x)) {
            result.terminated = true;
            result.steps = step;
            return result;
        }

        const int a = policy[x];
        if (a < 0 || a >= static_cast<int>(graph.nodes[x].actions.size())) {
            result.steps = step;
            return result;
        }

        double worst = -INF;
        Transition chosen;
        for (const auto& tr : graph.nodes[x].actions[a].trans) {
            const double next_value = graph.is_terminal(tr.to) ? 0.0 : J[tr.to];
            const double score = safe_add(tr.cost, next_value);
            if (score > worst + EPS ||
                (std::abs(score - worst) <= EPS && (chosen.to < 0 || tr.to < chosen.to))) {
                worst = score;
                chosen = tr;
            }
        }

        result.cost += chosen.cost;
        x = chosen.to;
        result.path.push_back(x);
    }

    result.steps = max_steps;
    result.terminated = graph.is_terminal(x);
    return result;
}

}  // namespace rsp
```
