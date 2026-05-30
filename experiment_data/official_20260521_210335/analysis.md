# Official Experiment Run 20260521_210335

本目录保存一次正式实验运行的数据。该归档已使用当前流水线（含修正后的随机图生成器，`rng_seed` 现按 `s` 独立）重新生成，因此其图文件名规范、CSV schema 与 `visualization/` 脚本完全一致，可直接用于复现与绘图。

## 实验 3：中规模效率比较

输入图：

- `exp3_runtime/graphs/`（文件名规范 `medium_n{n}_s2_a3_case{case}_seed{seed}.txt`，并随附 `graph_metadata.csv`）
- 规模：`n = 20, 50, 100, 200`
- 每种规模：20 张图
- 算法：`vi`, `pi`, `dijkstra`

输出：

- `exp3_runtime/results/runtime_experiment.csv`
- `exp3_runtime/results/runtime_summary.csv`（含 `requested_s, actions` 两列，本批均为 `requested_s=2, actions=3`）

摘要：

| n | algorithm | success_rate | avg_runtime_ms | avg_iterations | avg_value |
| --- | --- | ---: | ---: | ---: | ---: |
| 20 | dijkstra | 1.000000 | 0.006253 | 20.000000 | 16.172247 |
| 20 | pi | 1.000000 | 0.019130 | 3.400000 | 16.172247 |
| 20 | vi | 1.000000 | 0.006269 | 4.700000 | 16.172247 |
| 50 | dijkstra | 1.000000 | 0.024146 | 50.000000 | 25.341828 |
| 50 | pi | 1.000000 | 0.077516 | 4.400000 | 25.341828 |
| 50 | vi | 1.000000 | 0.018245 | 7.250000 | 25.341828 |
| 100 | dijkstra | 1.000000 | 0.077618 | 100.000000 | 25.947586 |
| 100 | pi | 1.000000 | 0.163161 | 4.800000 | 25.947586 |
| 100 | vi | 1.000000 | 0.035083 | 7.350000 | 25.947586 |
| 200 | dijkstra | 1.000000 | 0.274981 | 200.000000 | 25.350469 |
| 200 | pi | 1.000000 | 0.336960 | 5.000000 | 25.350469 |
| 200 | vi | 1.000000 | 0.068832 | 7.600000 | 25.350469 |

结论：

- 三个核心算法在 layered DAG 正权图上 success rate 都是 100%。
- 三个算法得到的 `avg_value` 完全一致，说明求得的是同一组 robust optimal values。
- 本实现和数据规模下，`vi` 整体 runtime 最小，在 `n >= 50` 上明显最快（最小规模 `n = 20` 时 `vi` 0.006269 与 `dijkstra` 0.006253 在亚微秒噪声内持平）；`dijkstra` 的 iterations 等于节点数，因为它每轮 finalized 一个节点；`pi` outer iterations 最少，但单轮 evaluation/proper check 成本较高。
- 全部 runtime 在亚毫秒量级；`pi` 与 `dijkstra` 处于同一量级，二者的细微排序对单次测量噪声较敏感（本次运行中 `dijkstra` 在 `n=200` 略快于 `pi`）。

## 实验 4：鲁棒性对比

输入图：

- `exp4_robustness/graphs/`（单次 `--successors-values 1 2 3 4 5` 生成，文件名规范 `medium_n50_s{s}_a3_case{case}_seed{seed}.txt`，并随附 `graph_metadata.csv`）
- 固定规模：`n = 50`
- 不确定 successor set size：`s = 1, 2, 3, 4, 5`
- 每个 `s`：20 张图
- 比较 policy：`baseline_nominal`, `baseline_bestcase`, `baseline_worst_immediate`, `vi`

输出：

- `exp4_robustness/results/robustness.csv`
- `exp4_robustness/results/robustness_summary.csv`

摘要（`avg_worst_cost` 只对 `policy_valid=true` 且 `terminated=true` 的样本求平均；本批各 policy 的 `valid_rate` 与 `terminated_rate` 均为 1.0）：

| s | policy_type | terminated_rate | avg_worst_cost | avg_steps |
| --- | --- | ---: | ---: | ---: |
| 1 | baseline_bestcase | 1.000000 | 15.891661 | 2.800000 |
| 1 | baseline_nominal | 1.000000 | 15.891661 | 2.800000 |
| 1 | baseline_worst_immediate | 1.000000 | 15.891661 | 2.800000 |
| 1 | vi | 1.000000 | 15.891661 | 2.800000 |
| 2 | baseline_bestcase | 1.000000 | 53.162185 | 5.550000 |
| 2 | baseline_nominal | 1.000000 | 48.044130 | 5.300000 |
| 2 | baseline_worst_immediate | 1.000000 | 42.076035 | 5.450000 |
| 2 | vi | 1.000000 | 35.661723 | 4.250000 |
| 3 | baseline_bestcase | 1.000000 | 79.942152 | 6.700000 |
| 3 | baseline_nominal | 1.000000 | 73.870759 | 6.100000 |
| 3 | baseline_worst_immediate | 1.000000 | 64.716992 | 6.350000 |
| 3 | vi | 1.000000 | 48.984977 | 4.900000 |
| 4 | baseline_bestcase | 1.000000 | 90.527246 | 6.850000 |
| 4 | baseline_nominal | 1.000000 | 87.050975 | 7.100000 |
| 4 | baseline_worst_immediate | 1.000000 | 77.985335 | 7.250000 |
| 4 | vi | 1.000000 | 57.829292 | 5.600000 |
| 5 | baseline_bestcase | 1.000000 | 102.228906 | 7.900000 |
| 5 | baseline_nominal | 1.000000 | 96.753060 | 7.550000 |
| 5 | baseline_worst_immediate | 1.000000 | 99.194495 | 8.400000 |
| 5 | vi | 1.000000 | 67.982757 | 6.450000 |

结论：

- `s=1` 时没有 successor 不确定性，所有 policy 的 worst-case cost 完全一致。
- 随着 `s` 增大，不确定性增强，deterministic baselines 的 worst-case cost 上升更快。
- `vi` robust policy 在所有 `s >= 2` 的平均 worst-case cost 都最低，体现了鲁棒规划在 adversarial rollout 下的优势。
- 所有 policy 在这批 layered DAG 图上都能终止，terminated rate 均为 100%；差异主要体现在 worst-case cost 和 steps 上。
- 由于生成器 `rng_seed` 现已包含 `s`，本批不同 `s` 的图来自独立随机流，跨 `s` 的对比不再共享随机前缀。

## 实验 4b：baseline 失效、配对统计与多种子稳定性

- `exp4b_trap/`：由 `experiments/generate_trap_graphs.py` 生成的陷阱图（n=3 gadget，存在 proper 策略但 nominal 捷径在对抗下成环）。20 张图上 `baseline_nominal`/`baseline_bestcase` 的 `valid_rate=0.15`、`baseline_worst_immediate=0.50`、`vi=1.00`——baseline 在对抗结构图上真正失效，而 robust VI 始终有效。
- 配对统计：在实验 4 的 100 张图上逐图比较，VI 的 worst-case cost 对每个 baseline 都不劣（**100/100**，0/300 落败）。
- `exp4_multiseed/seed{123,2024}/`：用 `seed ∈ {42,123,2024}` 重跑实验 4，三个种子下 VI 配对胜率均为 100/100（累计 900/900），worst-case cost 高度稳定（如 `s=5`：VI `66.97 ± 0.75` vs baseline ~93–102）。结论不依赖单一随机种子。
- 详见 [../../report/experiment_results.md](../../report/experiment_results.md) §4b。
