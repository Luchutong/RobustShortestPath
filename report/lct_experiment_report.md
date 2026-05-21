# lct 实验 3：中规模效率比较

## 实验目标

本实验比较三个核心 Robust Shortest Path 求解器在中规模图上的效率：

- `vi`: Value Iteration
- `pi`: Policy Iteration
- `dijkstra`: Dijkstra-like label-setting algorithm

比较指标包括 `runtime_ms`, `iterations`, `success`, `success_rate`, `avg_value`。

## 图生成方式

实验图由 `experiments/generate_medium_graphs.py` 生成。默认规模为：

```text
n = 20, 50, 100, 200
cases_per_n = 20
actions_per_node = 3
successors_per_action = 2
cost range = [1, 20]
```

生成器使用 layered DAG。每条转移只指向后续层或 terminal，因此图天然无环，并且每个非终点节点至少有一个 safe action 指向后续层，从而保证存在 proper policy。这样可以避免完全随机图不连通或无法保证到达终点的问题，让实验主要衡量算法效率。

## 运行命令

```bash
python3 experiments/generate_medium_graphs.py \
  --output data/random_graphs \
  --sizes 20 50 100 200 \
  --cases 20 \
  --actions 3 \
  --successors 2 \
  --seed 42

./build/run_runtime \
  --input-dir data/random_graphs \
  --output results \
  --epsilon 1e-9 \
  --max-iter 100000
```

输出文件：

- `results/runtime_experiment.csv`: 每张图、每个算法一行。
- `results/runtime_summary.csv`: 按 `n` 和 `algorithm` 聚合 success rate、平均 runtime、平均 iterations、平均 value。

## 指标定义

- `runtime_ms`: 直接调用 `run_algorithm` 的 wall-clock 时间，不包含进程启动开销。
- `iterations`: VI 的 Bellman update 次数、PI 的 outer iteration 次数、Dijkstra-like 的 finalized node 数。
- `success`: 算法是否成功完成。VI 必须达到收敛阈值才算成功。
- `success_rate`: 同一规模下成功图数除以总图数。
- `avg_value`: 非 terminal 节点 robust value 的平均值，只对成功结果汇总。

## 预期分析

在 layered DAG 且边权非负的图上，Dijkstra-like 通常最快，因为它每个节点只 finalized 一次。PI 的 outer iterations 通常较少，但每轮要做 policy evaluation 与 proper policy 检查。VI 最直接稳定，但需要多轮 Bellman update，runtime 可能随图层深度和规模增长。

`exhaustive` 不参与中规模效率比较，因为它枚举所有策略，只适合小图正确性验证。
