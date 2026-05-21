# Official Experiment Run 20260521_210335

本目录保存一次正式实验运行的数据。

## 实验 3：中规模效率比较

输入图：

- `exp3_runtime/graphs/`
- 规模：`n = 20, 50, 100, 200`
- 每种规模：20 张图
- 算法：`vi`, `pi`, `dijkstra`

输出：

- `exp3_runtime/results/runtime_experiment.csv`
- `exp3_runtime/results/runtime_summary.csv`

摘要：

| n | algorithm | success_rate | avg_runtime_ms | avg_iterations | avg_value |
| --- | --- | ---: | ---: | ---: | ---: |
| 20 | dijkstra | 1.000000 | 0.069289 | 20.000000 | 20.651649 |
| 20 | pi | 1.000000 | 0.190031 | 3.400000 | 20.651649 |
| 20 | vi | 1.000000 | 0.032947 | 4.900000 | 20.651649 |
| 50 | dijkstra | 1.000000 | 0.191933 | 50.000000 | 31.243825 |
| 50 | pi | 1.000000 | 0.439350 | 4.400000 | 31.243825 |
| 50 | vi | 1.000000 | 0.108514 | 7.150000 | 31.243825 |
| 100 | dijkstra | 1.000000 | 0.762531 | 100.000000 | 29.470691 |
| 100 | pi | 1.000000 | 0.924678 | 4.750000 | 29.470691 |
| 100 | vi | 1.000000 | 0.206872 | 7.050000 | 29.470691 |
| 200 | dijkstra | 1.000000 | 2.962888 | 200.000000 | 29.716002 |
| 200 | pi | 1.000000 | 1.914672 | 4.950000 | 29.716002 |
| 200 | vi | 1.000000 | 0.428881 | 7.300000 | 29.716002 |

结论：

- 三个核心算法在 layered DAG 正权图上 success rate 都是 100%。
- 三个算法得到的 `avg_value` 完全一致，说明求得的是同一组 robust optimal values。
- 本实现和数据规模下，`vi` runtime 最小；`dijkstra` 的 iterations 等于节点数，因为它每轮 finalized 一个节点；`pi` outer iterations 很少，但单轮 evaluation/proper check 成本较高。
- 随着 `n` 增大，三者 runtime 都上升；naive Dijkstra-like 在 `n=200` 时慢于 `vi` 和 `pi`，符合当前朴素扫描实现的复杂度特征。

## 实验 4：鲁棒性对比

输入图：

- `exp4_robustness/graphs/`
- 固定规模：`n = 50`
- 不确定 successor set size：`s = 1, 2, 3, 4, 5`
- 每个 `s`：20 张图
- 比较 policy：`baseline_nominal`, `baseline_bestcase`, `baseline_worst_immediate`, `vi`

输出：

- `exp4_robustness/results/robustness.csv`
- `exp4_robustness/results/robustness_summary.csv`

摘要：

| s | policy_type | terminated_rate | avg_worst_cost | avg_steps |
| --- | --- | ---: | ---: | ---: |
| 1 | baseline_bestcase | 1.000000 | 16.914130 | 2.450000 |
| 1 | baseline_nominal | 1.000000 | 16.914130 | 2.450000 |
| 1 | baseline_worst_immediate | 1.000000 | 16.914130 | 2.450000 |
| 1 | vi | 1.000000 | 16.914130 | 2.450000 |
| 2 | baseline_bestcase | 1.000000 | 59.333636 | 5.350000 |
| 2 | baseline_nominal | 1.000000 | 57.428797 | 5.000000 |
| 2 | baseline_worst_immediate | 1.000000 | 46.311814 | 5.100000 |
| 2 | vi | 1.000000 | 40.032566 | 4.450000 |
| 3 | baseline_bestcase | 1.000000 | 86.682949 | 6.600000 |
| 3 | baseline_nominal | 1.000000 | 84.505794 | 6.150000 |
| 3 | baseline_worst_immediate | 1.000000 | 65.758161 | 5.900000 |
| 3 | vi | 1.000000 | 52.233365 | 4.700000 |
| 4 | baseline_bestcase | 1.000000 | 101.003083 | 7.650000 |
| 4 | baseline_nominal | 1.000000 | 92.016128 | 7.200000 |
| 4 | baseline_worst_immediate | 1.000000 | 93.892031 | 7.550000 |
| 4 | vi | 1.000000 | 65.491315 | 5.550000 |
| 5 | baseline_bestcase | 1.000000 | 113.197262 | 7.900000 |
| 5 | baseline_nominal | 1.000000 | 111.235759 | 7.700000 |
| 5 | baseline_worst_immediate | 1.000000 | 106.653496 | 8.650000 |
| 5 | vi | 1.000000 | 76.368103 | 6.450000 |

结论：

- `s=1` 时没有 successor 不确定性，所有 policy 的 worst-case cost 完全一致。
- 随着 `s` 增大，不确定性增强，deterministic baselines 的 worst-case cost 上升更快。
- `vi` robust policy 在所有 `s >= 2` 的平均 worst-case cost 都最低，体现了鲁棒规划在 adversarial rollout 下的优势。
- 所有 policy 在这批 layered DAG 图上都能终止，terminated rate 均为 100%；差异主要体现在 worst-case cost 和 steps 上。
