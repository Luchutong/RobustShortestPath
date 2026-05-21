# Robust Shortest Path C++ 复现实验

本仓库用于复现 Bertsekas 的 Robust Shortest Path, RSP, 算法体系。我们计划完成：

- 实验 1：Toy Example，验证鲁棒策略相对普通最短路的优势。
- 实验 3：中规模效率比较，比较核心算法 runtime、iterations、success rate。
- 实验 4：鲁棒性对比，比较 deterministic baseline 与 robust policy 在 adversarial rollout 下的 worst-case cost。

代码协作原则：所有算法、baseline、实验脚本都通过统一接口读图、求解、输出 CSV。每个人可以独立实现自己的模块，但输出格式必须一致。

## 快速开始

```bash
cd /home/luchitong/算法实验/RobustShortestPath

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

运行 toy graph：

```bash
./build/rsp_main --input data/toy_graph.txt --algorithm vi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm dijkstra --output results
./build/rsp_main --input data/toy_graph.txt --algorithm pi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm exhaustive --output results
./build/rsp_main --input data/toy_graph.txt --algorithm baseline_nominal --output results
python3 experiments/generate_medium_graphs.py --output data/random_graphs
./build/run_runtime --input-dir data/random_graphs --output results
./build/run_robustness --input data/toy_graph.txt --output results --start 0 --max-steps 20
```

输出统一写到：

- `results/values.csv`
- `results/policies.csv`
- `results/residual_history.csv`
- `results/runtime.csv`
- `results/runtime_experiment.csv`
- `results/runtime_summary.csv`

## 小组分工

| 成员 | 核心算法 | Baseline | 实验 | 主要文件 |
| --- | --- | --- | --- | --- |
| hhm | Dijkstra-like Algorithm | Baseline 1: Deterministic Dijkstra baseline | 实验 4 鲁棒性对比 | `include/rsp/dijkstra_like.hpp`, `src/dijkstra_like.cpp`, `include/rsp/baseline.hpp`, `src/baseline.cpp`, `experiments/` |
| lct | Value Iteration, 值迭代 | Baseline 2: Exhaustive Search baseline | 实验 3 中规模效率比较、实验 4 robust VI policy | `include/rsp/value_iteration.hpp`, `src/value_iteration.cpp`, `include/rsp/exhaustive_search.hpp`, `src/exhaustive_search.cpp`, `experiments/` |
| csy | 剩余核心内容：Policy Iteration、proper policy、IO、实验整合 | 剩余 baseline / rollout / 对比汇总 | 实验 1、报告与可视化整合 | `include/rsp/policy_iteration.hpp`, `src/policy_iteration.cpp`, `include/rsp/proper_policy.hpp`, `src/proper_policy.cpp`, `src/io.cpp`, `visualization/`, `report/` |

实验 4 由 hhm 和 lct 共同完成：hhm 负责 deterministic baseline policy，lct 负责 robust VI policy，csy 负责把结果汇总进报告和图表。

## 统一接口总览

所有公共接口都在 `include/rsp/`。新增实现时先看 [docs/API.md](docs/API.md)，不要绕开这些接口。

### 1. 图结构接口

头文件：

```text
include/rsp/graph.hpp
```

核心类型：

```cpp
RobustGraph graph;
graph.n;              // 节点数，包含 terminal
graph.terminal;       // 终点编号
graph.nodes[x];       // 节点 x
graph.nodes[x].actions[a].trans; // action a 的所有 uncertain successors
```

统一约定：

- 节点编号为 `0..n-1`.
- `terminal` 是吸收终点。
- `policy[x]` 存 action 下标，不是 `action_id`.
- `policy[terminal] = -1`.

### 2. Bellman 公共接口

头文件：

```text
include/rsp/bellman.hpp
```

所有核心算法都应该复用：

```cpp
double action_value(const RobustGraph& graph, int x, int action_index, const std::vector<double>& J);
int greedy_action(const RobustGraph& graph, int x, const std::vector<double>& J);
std::vector<int> extract_greedy_policy(const RobustGraph& graph, const std::vector<double>& J);
std::vector<double> bellman_update(const RobustGraph& graph, const std::vector<double>& J);
```

其中：

```text
action_value(x,u,J) = max_y [g(x,u,y) + J(y)]
bellman_update(J)[x] = min_u action_value(x,u,J)
```

### 3. 算法统一运行接口

头文件：

```text
include/rsp/runner.hpp
```

实验脚本和命令行都应该调用：

```cpp
AlgorithmRunResult run_algorithm(
    const RobustGraph& graph,
    const std::string& algorithm_name,
    const AlgorithmOptions& options = {}
);
```

统一返回：

```cpp
struct AlgorithmRunResult {
    std::string name;
    AlgorithmKind kind;
    std::vector<double> value;
    std::vector<int> policy;
    std::vector<double> residual_history;
    int iterations;
    bool converged;
    bool success;
};
```

支持的算法名：

| 名称 | 类型 | 负责人 | 说明 |
| --- | --- | --- | --- |
| `vi` | core | lct | Value Iteration |
| `dijkstra` | core | hhm | Dijkstra-like Algorithm |
| `pi` | core | csy | Policy Iteration |
| `exhaustive` | baseline / ground truth | lct | 小规模枚举 proper policies |
| `baseline_nominal` | baseline | hhm | nominal successor deterministic baseline |
| `baseline_bestcase` | baseline | hhm | best-case successor deterministic baseline |
| `baseline_worst_immediate` | baseline | hhm | worst immediate cost deterministic baseline |

以后如果新增算法，只需要：

1. 在 `include/rsp/*.hpp` 定义结果结构和函数。
2. 在 `src/*.cpp` 实现。
3. 在 `src/runner.cpp` 的 `run_algorithm` 中注册算法名。
4. 在 [docs/API.md](docs/API.md) 更新接口说明。

### 4. IO 接口

头文件：

```text
include/rsp/io.hpp
```

统一读图：

```cpp
RobustGraph graph = read_graph_txt("data/toy_graph.txt");
```

统一输出：

```cpp
append_values_csv(...);
append_policies_csv(...);
append_residual_history_csv(...);
append_runtime_csv(...);
```

TXT 和 CSV 格式见 [docs/INPUT_OUTPUT.md](docs/INPUT_OUTPUT.md)。

### 5. Proper Policy 接口

头文件：

```text
include/rsp/proper_policy.hpp
```

PI、exhaustive search、最终 policy 检查都应该使用：

```cpp
ProperPolicyResult find_initial_proper_policy(const RobustGraph& graph);
PolicyCheckResult check_policy_proper(const RobustGraph& graph, const std::vector<int>& policy);
std::vector<double> evaluate_proper_policy_dag(const RobustGraph& graph, const std::vector<int>& policy);
```

## 三个实验如何使用统一接口

### 实验 1：Toy Example

目标：

- 验证 `vi`, `dijkstra`, `pi`, `exhaustive` 在 toy graph 上得到同样结果。
- 展示 nominal baseline 在对抗 successor 下可能失败。

命令：

```bash
./build/rsp_main --input data/toy_graph.txt --algorithm vi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm dijkstra --output results
./build/rsp_main --input data/toy_graph.txt --algorithm pi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm exhaustive --output results
./build/rsp_main --input data/toy_graph.txt --algorithm baseline_nominal --output results
```

期望 robust value：

```text
J(0)=7, J(1)=1, J(2)=101, J(3)=4, J(4)=100, J(5)=0
```

### 实验 3：中规模效率比较

目标：

- 对 `vi`, `pi`, `dijkstra` 运行同一批随机图。
- 比较 `runtime_ms`, `iterations`, `success`, `success_rate`, `avg_value`.

生成中规模 layered DAG 随机图：

```bash
python3 experiments/generate_medium_graphs.py \
  --output data/random_graphs \
  --sizes 20 50 100 200 \
  --cases 20 \
  --actions 3 \
  --successors 2 \
  --seed 42
```

批量运行效率实验：

```bash
./build/run_runtime \
  --input-dir data/random_graphs \
  --output results \
  --epsilon 1e-9 \
  --max-iter 100000
```

输出：

- `results/runtime_experiment.csv`
- `results/runtime_summary.csv`

### 实验 4：鲁棒性对比

目标：

- hhm 生成 deterministic baseline policy。
- lct 生成 robust VI policy。
- 对两类 policy 做 adversarial rollout，比较 worst-case cost。

统一 rollout 接口：

```cpp
RolloutResult adversarial_rollout(
    const RobustGraph& graph,
    const std::vector<int>& policy,
    const std::vector<double>& J,
    int start,
    int max_steps
);
```

建议比较对象：

```text
baseline_nominal
baseline_bestcase
baseline_worst_immediate
vi
```

命令：

```bash
./build/run_robustness --input data/toy_graph.txt --output results --start 0 --max-steps 20
```

如果批量实验中 `s` 是图生成器控制的实验参数，可以显式传入：

```bash
./build/run_robustness --input data/toy_graph.txt --output results --start 0 --max-steps 20 --s 2
```

不传 `--s` 时，程序默认使用图中最大 successor set size。

结果建议输出：

```csv
graph_id,s,policy_type,start_node,worst_cost,terminated,steps
```

## 仓库结构

```text
include/rsp/       公共 C++ 接口，所有成员以这里为准
src/               算法、baseline、runner、IO 实现
data/              toy graph 和后续实验图
tests/             单元测试
experiments/       随机图生成和批量实验入口
visualization/     Python 可视化脚本
results/           CSV/JSON 运行结果
report/            报告、证明和图片
docs/              API、输入输出、协作规范
```

## 提交前检查

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

如果修改了 `include/rsp/*.hpp`、`src/runner.cpp` 或命令行参数，必须同步更新 [docs/API.md](docs/API.md) 和 [docs/INPUT_OUTPUT.md](docs/INPUT_OUTPUT.md)。
