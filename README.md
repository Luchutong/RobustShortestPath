# Robust Shortest Path C++ 复现实验

本仓库用于小组协作复现 Bertsekas 的 Robust Shortest Path, RSP, 相关算法。项目目标是实现带对抗性 successor uncertainty 的最短路规划，并比较 Value Iteration、Policy Iteration、Dijkstra-like algorithm、exhaustive search 与 deterministic baselines。

## 快速开始

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure

./build/rsp_main --input data/toy_graph.txt --algorithm vi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm pi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm dijkstra --output results
./build/rsp_main --input data/toy_graph.txt --algorithm exhaustive --output results
```

运行后会生成：

- `results/values.csv`
- `results/policies.csv`
- `results/residual_history.csv`
- `results/runtime.csv`

## 仓库结构

```text
include/rsp/       公共 C++ 接口，所有成员以这里为准
src/               算法与 IO 实现
data/              toy graph 和后续实验图
tests/             单元测试
experiments/       随机图生成和批量实验入口
visualization/     Python 可视化脚本
results/           CSV/JSON 运行结果
report/            报告、证明和图片
docs/              统一接口文档、输入输出规范、协作规范
```

## 小组分工建议

- 成员 A：理论建模、Bellman 方程、proper policy 与正确性说明，主要维护 `report/` 和 `docs/`.
- 成员 B：C++ 算法实现和测试，主要维护 `include/rsp/`, `src/`, `tests/`.
- 成员 C：实验生成、CSV 汇总和可视化，主要维护 `experiments/`, `visualization/`, `results/`, `report/figures/`.

## 统一接口

先阅读 [docs/API.md](docs/API.md)。任何算法模块新增结果字段或修改函数签名，都需要同步更新该文档和对应头文件。

## 输入格式

默认使用 TXT 格式，避免第一阶段引入第三方 JSON 依赖。格式详见 [docs/INPUT_OUTPUT.md](docs/INPUT_OUTPUT.md)。

