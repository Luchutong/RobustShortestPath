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

## 实测结果

用当前流水线生成 80 张图（`n ∈ {20,50,100,200}`，每规模 20 张，`actions=3`，`successors=2`，`seed=42`）并运行 `run_runtime`，得到 [experiment_data/official_20260521_210335/exp3_runtime/results/runtime_summary.csv](../experiment_data/official_20260521_210335/exp3_runtime/results/runtime_summary.csv)（本批 `requested_s=2, actions=3`）：

| n | algorithm | success_rate | avg_runtime_ms | avg_iterations | avg_value |
| --- | --- | ---: | ---: | ---: | ---: |
| 20 | vi | 1.000000 | 0.006269 | 4.700000 | 16.172247 |
| 20 | pi | 1.000000 | 0.019130 | 3.400000 | 16.172247 |
| 20 | dijkstra | 1.000000 | 0.006253 | 20.000000 | 16.172247 |
| 50 | vi | 1.000000 | 0.018245 | 7.250000 | 25.341828 |
| 50 | pi | 1.000000 | 0.077516 | 4.400000 | 25.341828 |
| 50 | dijkstra | 1.000000 | 0.024146 | 50.000000 | 25.341828 |
| 100 | vi | 1.000000 | 0.035083 | 7.350000 | 25.947586 |
| 100 | pi | 1.000000 | 0.163161 | 4.800000 | 25.947586 |
| 100 | dijkstra | 1.000000 | 0.077618 | 100.000000 | 25.947586 |
| 200 | vi | 1.000000 | 0.068832 | 7.600000 | 25.350469 |
| 200 | pi | 1.000000 | 0.336960 | 5.000000 | 25.350469 |
| 200 | dijkstra | 1.000000 | 0.274981 | 200.000000 | 25.350469 |

综合对比图见 [runtime_comparison.svg](figures/runtime_comparison.svg)。观测结论：

- 三个算法 success rate 全部 100%，且 `avg_value` 在同一规模下完全一致，说明三者求得同一组 robust optimal values。
- `vi` 在 `n >= 50` 上 runtime 最小、整体最快；`n = 20` 时 `vi`(0.006269) 与 `dijkstra`(0.006253) 在亚微秒噪声内几乎持平。
- `pi` 外层迭代最少（约 3~5 轮），但单轮 policy evaluation + properness 检查开销较高。
- `dijkstra` 的 `iterations` 恒等于节点数；其 runtime 与 `pi` 处于同一量级，二者细微排序对亚毫秒级测量噪声较敏感。

## 预期与实测对照

事前预期：在 layered DAG 且边权非负的图上，Dijkstra-like 因为每个节点只 finalized 一次而“可能最快”；PI 的 outer iterations 较少；VI 需要多轮 Bellman update。

实测修正：本仓库的 Dijkstra-like 是 naive 扫描实现（每轮 `O(n·A·S)` 重扫所有未确定节点，总体约 `O(n²·A·S)`），因此在这些规模上并不比 VI 快；**VI 在 `n >= 50` 上最快、整体也最快**（`n = 20` 与 dijkstra 在亚微秒噪声内持平），因为它只需少量整体 sweep 即收敛。PI 的外层迭代数确实最少，但单轮开销高，总时间与 Dijkstra-like 同量级。也就是说迭代次数少 ≠ 总时间短，实现常数与单轮成本同样关键。

`exhaustive` 不参与中规模效率比较，因为它枚举所有策略，只适合小图正确性验证。
