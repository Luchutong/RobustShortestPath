# 统一接口文档

本文件是小组协作时的接口契约。实现可以优化，但函数签名、结果字段含义和输入输出语义需要保持稳定。

## 1. 核心数据结构

头文件：`include/rsp/graph.hpp`

```cpp
struct Transition {
    int to;
    double cost;
};

struct Action {
    int action_id;
    std::string name;
    std::vector<Transition> trans;
};

struct Node {
    int id;
    std::vector<Action> actions;
};

class RobustGraph {
public:
    int n;
    int terminal;
    std::vector<Node> nodes;

    bool is_terminal(int x) const;
    int total_actions() const;
    int total_transitions() const;
    void validate() const;
};
```

约定：

- 节点编号为 `0..n-1`.
- `terminal` 是终点编号。
- `nodes[terminal].actions` 应为空。
- `policy[x]` 存的是 `nodes[x].actions` 的下标，不是输入文件中的 `action_id`.
- `policy[terminal] = -1`.

## 2. 数值约定

头文件：`include/rsp/utils.hpp`

```cpp
constexpr double INF = 1e100;
constexpr double EPS = 1e-9;
bool is_inf(double x);
double safe_add(double a, double b);
double finite_abs_diff(double a, double b);
bool less_with_eps(double a, double b);
```

约定：

- CSV 中无穷值输出为 `inf`.
- 不使用 `std::numeric_limits<double>::infinity()` 存储算法中的无穷值。
- 正确性比较默认阈值为 `1e-8`.

## 3. Bellman 接口

头文件：`include/rsp/bellman.hpp`

```cpp
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
```

语义：

```text
action_value(x,u,J) = max_y [g(x,u,y) + J(y)]
bellman_update(J)[x] = min_u action_value(x,u,J)
```

终点恒有 `J[terminal] = 0`.

## 4. Value Iteration

头文件：`include/rsp/value_iteration.hpp`

```cpp
struct ValueIterationResult {
    std::vector<double> value;
    std::vector<int> policy;
    int iterations;
    bool converged;
    std::vector<double> residual_history;
    std::vector<std::vector<double>> value_history;
};

ValueIterationResult value_iteration(
    const RobustGraph& graph,
    double epsilon = 1e-9,
    int max_iter = 100000,
    bool init_with_inf = true,
    bool save_history = true
);
```

约定：

- `init_with_inf=true` 用于主实验效率比较。
- `init_with_inf=false` 用于画收敛曲线。
- `policy` 是最终 value 上提取出的 greedy policy，使用前建议调用 `check_policy_proper`.

## 5. Proper Policy

头文件：`include/rsp/proper_policy.hpp`

```cpp
struct ProperPolicyResult {
    bool exists;
    std::vector<int> policy;
    std::vector<int> rank;
};

struct PolicyCheckResult {
    bool proper;
    bool has_cycle;
    bool all_reach_terminal;
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
```

语义：

- `find_initial_proper_policy` 使用 reachability 思想构造初始 proper policy。
- `check_policy_proper` 要同时检查非终点环和是否都能到达 terminal。
- `evaluate_proper_policy_dag` 只接受 proper policy，否则抛异常。

## 6. Policy Iteration

头文件：`include/rsp/policy_iteration.hpp`

```cpp
struct PolicyIterationResult {
    std::vector<double> value;
    std::vector<int> policy;
    int outer_iterations;
    bool converged;
    bool final_policy_proper;
    std::vector<int> policy_change_history;
    std::vector<double> residual_history;
};

PolicyIterationResult policy_iteration(
    const RobustGraph& graph,
    int max_outer_iter = 10000,
    double epsilon = 1e-9
);
```

约定：

- 从 `find_initial_proper_policy` 开始。
- 每轮 improvement 后必须检查 properness。
- tie-breaking 默认保留旧 action，减少策略震荡。

## 7. Dijkstra-like

头文件：`include/rsp/dijkstra_like.hpp`

```cpp
struct DijkstraLikeResult {
    std::vector<double> value;
    std::vector<int> policy;
    bool success;
    int finalized_count;
    std::vector<int> finalize_order;
};

DijkstraLikeResult dijkstra_like(const RobustGraph& graph);
```

约定：

- 第一版实现 naive label-setting。
- 只有当 action 的所有 successors 已经 finalized 时，才可以计算该 action 的 candidate。
- 如果无法继续确定新节点，返回 `success=false`.

## 8. Exhaustive Search

头文件：`include/rsp/exhaustive_search.hpp`

```cpp
struct ExhaustiveSearchResult {
    std::vector<double> value;
    std::vector<int> policy;
    bool success;
    int checked_policies;
    int proper_policies;
};

ExhaustiveSearchResult exhaustive_search(const RobustGraph& graph);
```

用途：

- 只用于小规模图正确性验证。
- `value[x]` 表示所有 proper policies 中对应节点的最小 worst-case cost。

## 9. Baseline 与 Rollout

头文件：`include/rsp/baseline.hpp`

```cpp
enum class DeterministicMode {
    Nominal,
    BestCase,
    WorstImmediate
};

struct BaselineResult {
    std::vector<double> value;
    std::vector<int> policy;
    bool success;
};

struct RolloutResult {
    std::vector<int> path;
    double cost;
    bool terminated;
    int steps;
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
```

说明：

- baseline 不代表精确 RSP 求解器，只用于展示 nominal planning 在对抗环境下的风险。
- `adversarial_rollout` 每步选择 `argmax_y [g + J(y)]`.

## 10. IO

头文件：`include/rsp/io.hpp`

```cpp
RobustGraph read_graph_txt(const std::string& path);
void write_graph_json(const RobustGraph& graph, const std::string& path);

void append_values_csv(...);
void append_policies_csv(...);
void append_residual_history_csv(...);
void append_runtime_csv(...);
```

输出字段见 [INPUT_OUTPUT.md](INPUT_OUTPUT.md)。

