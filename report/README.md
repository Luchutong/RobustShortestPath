# Report

建议报告文件：

- `theory_notes.md`
- `proof_section.md`
- `experiment_results.md`
- `lct_experiment_report.md`

建议最终报告结构见原实验指导书第十五节。图片统一放在 `report/figures/`。

当前仓库已补充：

- `theory_notes.md`：RSP 问题定义、Bellman 方程与算法关系的理论笔记。
- `proof_section.md`：proper policy、VI/PI 正确性与实验设计可直接扩写的证明提纲。
- `experiment_results.md`：整合 toy example、效率实验、鲁棒性对比的总报告骨架。

推荐流程：

1. 先运行 C++ 程序生成 `results/*.csv`。
2. 再运行 `visualization/*.py` 生成图片到 `report/figures/`。
3. 最后把图插入 `experiment_results.md` 或组内最终总报告。
