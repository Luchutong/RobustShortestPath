# Visualization

Python 可视化脚本放在这里。脚本只读取 `data/*.json` 和 `results/*.csv`，不重新实现算法。

建议脚本：

- `plot_graph.py`: action-node graph.
- `plot_values.py`: value table or bar chart.
- `plot_comparison.py`: runtime and correctness comparison.
- `plot_policy.py`: final robust policy graph.
- `plot_toy_steps.py`: nominal path, adversarial path, robust policy.

图片输出到 `report/figures/`。

当前目录已补充上述五个脚本，以及公共工具 `common.py`。

推荐命令：

```bash
python3 visualization/plot_graph.py \
	--graph-json data/toy_graph.json \
	--output report/figures/toy_graph.svg

python3 visualization/plot_values.py \
	--values-csv results/values.csv \
	--graph-id toy_graph \
	--algorithms vi,pi,dijkstra,exhaustive \
	--output report/figures/toy_values.svg

python3 visualization/plot_policy.py \
	--graph-json data/toy_graph.json \
	--policies-csv results/policies.csv \
	--values-csv results/values.csv \
	--graph-id toy_graph \
	--algorithm vi \
	--output report/figures/toy_policy_vi.svg

python3 visualization/plot_toy_steps.py \
	--graph-json data/toy_graph.json \
	--policies-csv results/policies.csv \
	--values-csv results/values.csv \
	--robustness-csv results/robustness.csv \
	--graph-id toy_graph \
	--output report/figures/toy_steps.svg

python3 visualization/plot_comparison.py \
	--runtime-summary-csv results/runtime_summary.csv \
	--robustness-summary-csv results/robustness_summary.csv \
	--output report/figures/runtime_comparison.svg
```

依赖：

- Python 3

脚本默认输出 SVG，不依赖 matplotlib、pandas 或 networkx。

