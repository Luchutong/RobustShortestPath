# Experiments

这里放随机图生成器和批量实验脚本。

建议优先级：

1. `generate_graph.cpp` 或 Python 生成 layered graph，保证存在 proper policy。
2. `run_correctness.sh` 跑小规模图，与 exhaustive search 对比。
3. `run_runtime.sh` 跑中规模图，比较 VI、PI、Dijkstra-like。
4. `run_robustness.sh` 改变 successor set size，比较 robust policy 与 deterministic baseline。

所有实验输出统一写入 `results/`。

