# Experiments

这里放随机图生成器和批量实验脚本。

计划完成：

1. 实验 1：Toy Example，由 csy 汇总，调用 `vi`, `dijkstra`, `pi`, `exhaustive`, `baseline_nominal`.
2. 实验 3：中规模效率比较，由 lct 汇总，调用 `vi`, `pi`, `dijkstra`.
3. 实验 4：鲁棒性对比，由 hhm 和 lct 共同完成，比较 deterministic baselines 与 robust VI policy.

成员接口：

- hhm：提供 `dijkstra`, `baseline_nominal`, `baseline_bestcase`, `baseline_worst_immediate`.
- lct：提供 `vi`, `exhaustive`, 实验 3 图生成与 runtime 汇总。
- csy：提供 `pi`, proper policy / IO / 实验 1 汇总。

建议脚本：

1. `run_toy.sh`: 跑实验 1 的 toy graph。
2. `generate_medium_graphs.py`: 生成实验 3 的中规模 layered DAG 图。
3. `run_runtime`: C++ 入口，跑实验 3 的中规模效率比较。
4. `run_robustness`: C++ 入口，跑实验 4 的鲁棒性对比。

所有实验输出统一写入 `results/`。

实验 3 当前命令：

```bash
# 单 successor 值：
python3 experiments/generate_medium_graphs.py \
  --output data/random_graphs \
  --sizes 20 50 100 200 \
  --cases 20 \
  --actions 3 \
  --successors 2 \
  --seed 42

# 多 successor 值（一次生成所有 s，避免 metadata 覆盖）：
python3 experiments/generate_medium_graphs.py \
  --output data/random_graphs \
  --sizes 20 \
  --cases 20 \
  --actions 3 \
  --successors-values 1 2 3 4 5 \
  --seed 42

./build/run_runtime \
  --input-dir data/random_graphs \
  --output results \
  --epsilon 1e-9 \
  --max-iter 100000
```

输出：

```text
results/runtime_experiment.csv
results/runtime_summary.csv
results/graph_metadata.csv
```

`runtime_experiment.csv` 字段：

```csv
graph_id,n,total_actions,total_transitions,algorithm,runtime_ms,iterations,converged,success,avg_value
```

`runtime_summary.csv` 字段：

```csv
n,requested_s,actions,algorithm,cases,success_count,success_rate,avg_runtime_ms,avg_iterations,avg_value
```

`graph_metadata.csv` 字段：

```csv
graph_id,n,actions,case,base_seed,display_seed,rng_seed,requested_s,min_actual_s,max_actual_s,avg_actual_s,min_cost,max_cost
```

当一次运行生成多个 `requested_s` 时，脚本还会同时写出按本次请求集合命名的 metadata，例如：

```text
graph_metadata_s1_2_5.csv
```

`graph_metadata.csv` 代表本次最新 generator 运行的合并 metadata；若多次向同一目录生成图，旧的 `graph_metadata.csv` 会被覆盖。若需要保留每次运行的独立归档，请使用 `graph_metadata_s*.csv`。

并且当前 generator 会拒绝重复的 `--sizes` 或重复的 `--successors-values`，避免图文件与 metadata 行被静默覆盖。

实验 4 当前命令：

```bash
./build/run_robustness --input data/toy_graph.txt --output results --start 0 --max-steps 20
```

批量运行正式鲁棒性对比：

```bash
./build/run_robustness \
  --input-dir experiment_data/official_20260521_210335/exp4_robustness/graphs \
  --output results \
  --start 0 \
  --max-steps 1000
```

如果实验图的 successor count 上界是外部控制变量，可以显式传入：

```bash
./build/run_robustness --input data/toy_graph.txt --output results --start 0 --max-steps 20 --s 2
```

不传 `--s` 时，程序默认使用图中最大 distinct successor count 作为该图的 `s` 标记。对于 layered DAG 生成图，这里的 `s` 更接近“每个 action 不同 successor 数量的请求上界”。若显式传入 `--s`，其值必须为正；目录模式下不允许再传 `--s`。

输出 `results/robustness.csv`：

```csv
graph_id,s,policy_type,start_node,policy_valid,status,worst_cost,terminated,steps
```

批量运行时还会额外输出 `results/robustness_summary.csv`：

```csv
s,policy_type,cases,valid_count,valid_rate,terminated_count,terminated_rate,avg_worst_cost,avg_steps
```

实验 4b 陷阱图（演示 baseline 真正失效）：

`generate_medium_graphs.py` 生成的 layered DAG 在结构上保证每个 policy 都 proper（`valid_rate` 恒为 1）。`generate_trap_graphs.py` 则生成一类"陷阱"图——存在 proper 策略，但便宜的 nominal 捷径在对抗后继下成环，使 deterministic baseline 产出 improper 策略：

```bash
python3 experiments/generate_trap_graphs.py --output data/trap_graphs --cases 20 --seed 42
./build/run_robustness --input-dir data/trap_graphs --output results --start 0 --max-steps 200
```

结果中 `baseline_nominal` / `baseline_bestcase` 的 `valid_rate` 显著小于 1（多数图 improper），而 `vi` 恒为 1，定量地展示鲁棒规划相对 deterministic baseline 的优势（详见 [report/experiment_results.md](../report/experiment_results.md) §4b）。
