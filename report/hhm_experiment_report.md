# hhm 实验报告：RSP Dijkstra-like 与 Deterministic Baseline 鲁棒性对比

## 1. 实验目标

本实验围绕 Bertsekas 的 Robust Shortest Path, RSP, 问题中 hhm 负责的工程部分展开，目标是完成并验证：

1. 非负代价条件下的 Dijkstra-like label-setting 算法；
2. 三种 deterministic Dijkstra baseline；
3. 实验 4 中 deterministic baseline policy 与 robust VI policy 的 adversarial rollout 对比。

RSP 与普通最短路的关键区别是：决策者在节点 `x` 选择 action `u` 后，不能直接指定唯一后继，而是由对抗者从 `Y(x,u)` 中选择最坏 successor。因此算法输出不是单条路径，而是每个节点对应 action 的 policy。

## 2. 实现内容

### 2.1 Dijkstra-like Algorithm

实现文件：

- `include/rsp/dijkstra_like.hpp`
- `src/dijkstra_like.cpp`

实现采用指导书建议的 naive label-setting 版本：

1. 初始永久集合只包含 terminal；
2. 每轮扫描所有未确定节点；
3. 只有当某个 action 的所有 successors 都已经 finalized，才计算该 action 的 robust candidate：

```text
candidate(x,u) = max_{y in Y(x,u)} [g(x,u,y) + J(y)]
```

4. 在所有可计算 candidate 中选择最小者永久标号；
5. 若没有任何节点可继续确定，则返回 `success=false`。

本次补充了两个重要工程约束：

- Dijkstra-like 只适用于非负转移代价；发现负代价时直接返回 `success=false`；
- candidate 值相同时，固定按较小 node id、再按较小 action index 进行 tie-breaking，保证结果可复现。

### 2.2 Deterministic Dijkstra Baselines

实现文件：

- `include/rsp/baseline.hpp`
- `src/baseline.cpp`

三种 baseline 都不是精确 RSP 求解器，而是用于展示普通 deterministic planning 在对抗 successor 下的风险。

本次实现不再只看 immediate cost，而是先把 RSP 图确定化，再从 terminal 反向执行 shortest-path planning：

| baseline | 确定化规则 |
| --- | --- |
| `baseline_nominal` | 每个 action 只使用第一个 successor |
| `baseline_bestcase` | 把 action 的所有 successors 都视为 deterministic planner 可选边 |
| `baseline_worst_immediate` | 每个 action 只使用 immediate cost 最大的 successor |

确定化规划得到 action policy 后，再把该 policy 放回原 RSP 图，用 `evaluate_proper_policy_dag` 计算真实 adversarial worst-case value。若得到的 policy 不是 proper，则 `success=false`。

### 2.3 Experiment 4 Robustness Runner

新增文件：

- `experiments/run_robustness.cpp`

命令：

```bash
./build/run_robustness --input data/toy_graph.txt --output results --start 0 --max-steps 20
```

程序会在 rollout 前检查 `run.success`、`run.converged` 和 policy properness。若某个算法没有产生可用于鲁棒性比较的 proper policy，则该行输出 `inf` cost，避免把无效 policy 当成 robust 结果。

该程序统一运行：

```text
baseline_nominal
baseline_bestcase
baseline_worst_immediate
vi
```

然后对每个 policy 从指定 start node 执行 adversarial rollout，输出：

```csv
graph_id,s,policy_type,start_node,worst_cost,terminated,steps
```

其中 `s` 取图中最大 successor set size，用来表示该图中的不确定性规模。
如果批量实验已经由图生成器明确控制了 successor set size，也可以通过 `--s` 显式传入实验参数。

## 3. Toy Graph 实验

实验使用仓库中的 `data/toy_graph.txt`。该图中节点 0 有两个 action：

- `fast_risky`: successor 为 `{1,2}`，名义上看起来很短；
- `safe`: successor 为 `{3}`，当前代价更高但 worst-case 更稳定。

手算 robust value：

```text
J(5) = 0
J(1) = 1
J(4) = 100
J(2) = 101
J(3) = 4

fast_risky at 0 = max(1 + J(1), 1 + J(2)) = 102
safe at 0       = 3 + J(3) = 7

J(0) = 7
```

因此 robust policy 在节点 0 应选择 `safe`，而 deterministic baseline 会倾向于选择 `fast_risky`。

## 4. 实验结果

在远程主机 `xueshuoshuo_m25` 上运行：

```bash
export PATH="$HOME/.local/bin:$PATH"
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/run_robustness --input data/toy_graph.txt --output /tmp/rsp-final-smoke --start 0 --max-steps 20
```

单元测试结果：

```text
2/2 tests passed
```

鲁棒性对比输出：

```csv
graph_id,s,policy_type,start_node,worst_cost,terminated,steps
toy_graph,2,baseline_nominal,0,102.000000,1,3
toy_graph,2,baseline_bestcase,0,102.000000,1,3
toy_graph,2,baseline_worst_immediate,0,102.000000,1,3
toy_graph,2,vi,0,7.000000,1,2
```

结果说明：

- 三种 deterministic baseline 在 toy graph 上都选择节点 0 的 `fast_risky` action；
- 对抗者会在 `fast_risky` 的 successors 中选择通向节点 2 的路径，最终 worst-case cost 为 `102`；
- robust VI policy 在节点 0 选择 `safe` action，worst-case cost 为 `7`；
- 该结果复现了指导书中“普通最短路在不确定环境下过于乐观，而 RSP policy 更保守但更鲁棒”的核心现象。

## 5. 正确性检查

新增测试文件 `tests/test_hhm.cpp` 覆盖以下内容：

1. toy graph 中 Dijkstra-like 返回 `J(0)=7`，且节点 0 选择 safe action；
2. Dijkstra-like 对无法继续 finalized 的图返回 `success=false`；
3. Dijkstra-like 对负代价图返回 `success=false`，明确其适用条件；
4. 三种 deterministic baseline 在 toy graph 上选择 `fast_risky`；
5. nominal baseline 的 adversarial rollout cost 为 `102`，robust VI policy 的 adversarial rollout cost 为 `7`；
6. 构造一个反例，验证 baseline 不是 immediate-cost 贪心，而是真正按完整 deterministic shortest-path distance 做规划。
7. 构造一个 mode-separation 图，验证 `Nominal`、`BestCase`、`WorstImmediate` 三种 baseline 能在同一节点选择不同 action。

## 6. 与指导书要求的对应关系

| 指导书要求 | 当前完成情况 |
| --- | --- |
| Dijkstra-like naive label-setting | 已完成 |
| action 的所有 successors finalized 后才能更新 | 已完成 |
| 非负边权适用条件 | 已在代码和文档中明确 |
| deterministic nominal baseline | 已完成 |
| deterministic best-case baseline | 已完成 |
| deterministic worst-immediate baseline | 已完成 |
| rollout 前验证 policy 有效性 | 已完成 |
| adversarial rollout | 已复用并用于实验 4 |
| 实验 4 robustness.csv | 已完成 |
| 单元测试 | 已补充 `test_hhm` |
| 文档与命令说明 | 已更新 README / API / INPUT_OUTPUT / experiments README |

需要注意的是，本报告只覆盖 hhm 负责的模块。指导书中完整项目还包括 VI、PI、exhaustive search、随机图生成、可视化和最终报告整合等内容，其中部分属于其他成员职责或仓库已有实现，不在本次 hhm 分支中扩展。

## 7. 结论

本次实现完成了 hhm 负责的 RSP Dijkstra-like、deterministic baseline 和实验 4 鲁棒性对比部分。toy graph 结果表明，deterministic baseline 的名义最短策略在 adversarial rollout 下 worst-case cost 达到 `102`，而 robust VI policy 的 worst-case cost 为 `7`。这说明在存在 set-membership transition uncertainty 的最短路问题中，输出鲁棒 policy 比输出普通 deterministic path 更符合 RSP 问题定义。
