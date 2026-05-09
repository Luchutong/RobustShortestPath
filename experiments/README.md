# Experiments

这里放随机图生成器和批量实验脚本。

计划完成：

1. 实验 1：Toy Example，由 csy 汇总，调用 `vi`, `dijkstra`, `pi`, `exhaustive`, `baseline_nominal`.
2. 实验 3：中规模效率比较，由 csy 汇总，调用 `vi`, `pi`, `dijkstra`.
3. 实验 4：鲁棒性对比，由 hhm 和 lct 共同完成，比较 deterministic baselines 与 robust VI policy.

成员接口：

- hhm：提供 `dijkstra`, `baseline_nominal`, `baseline_bestcase`, `baseline_worst_immediate`.
- lct：提供 `vi`, `exhaustive`.
- csy：提供 `pi`, proper policy / IO / 结果汇总脚本。

建议脚本：

1. `run_toy.sh`: 跑实验 1 的 toy graph。
2. `run_runtime.sh`: 跑实验 3 的中规模图。
3. `run_robustness.sh`: 跑实验 4 的鲁棒性对比。
4. `generate_graph.cpp` 或 Python 生成 layered graph，保证存在 proper policy。

所有实验输出统一写入 `results/`。
