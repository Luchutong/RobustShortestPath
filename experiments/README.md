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

输出：

```text
results/runtime_experiment.csv
results/runtime_summary.csv
```

`runtime_experiment.csv` 字段：

```csv
graph_id,n,total_actions,total_transitions,algorithm,runtime_ms,iterations,converged,success,avg_value
```

`runtime_summary.csv` 字段：

```csv
n,algorithm,cases,success_count,success_rate,avg_runtime_ms,avg_iterations,avg_value
```

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

不传 `--s` 时，程序默认使用图中最大 successor count 作为该图的 `s` 标记。对于 layered DAG 生成图，这里的 `s` 更接近“每个 action 不同 successor 数量的请求上界”。

输出 `results/robustness.csv`：

```csv
graph_id,s,policy_type,start_node,policy_valid,status,worst_cost,terminated,steps
```

批量运行时还会额外输出 `results/robustness_summary.csv`：

```csv
s,policy_type,cases,valid_count,valid_rate,terminated_count,terminated_rate,avg_worst_cost,avg_steps
```
