# 协作规范

## 分支建议

```text
main
feature/vi
feature/pi
feature/dijkstra
feature/baseline-dijkstra
feature/exhaustive
feature/experiments
feature/visualization
docs/report
```

每个 PR 尽量只做一类事情：算法、测试、实验脚本、可视化或报告。

## 提交前检查

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

如果只修改文档，可以在 PR 里说明未运行 C++ 测试。

## 接口变更规则

修改以下文件时，需要同步更新 `docs/API.md`：

```text
include/rsp/*.hpp
src/main.cpp
docs/INPUT_OUTPUT.md
```

## 代码风格

- C++ 标准：C++17.
- 命名空间：所有库代码放在 `namespace rsp`.
- 无穷值：使用 `rsp::INF`.
- 浮点比较：使用 `rsp::EPS` 或 `less_with_eps`.
- 新算法应返回结果结构体，不直接打印。
- C++ 负责生成 CSV/JSON，Python 只读取结果并画图。

## 当前任务拆分

hhm：

- Dijkstra-like Algorithm：`include/rsp/dijkstra_like.hpp`, `src/dijkstra_like.cpp`
- Baseline 1 deterministic Dijkstra：`include/rsp/baseline.hpp`, `src/baseline.cpp`
- 实验四中的 deterministic baseline policy 和 adversarial rollout 对比数据

lct：

- Value Iteration：`include/rsp/value_iteration.hpp`, `src/value_iteration.cpp`
- Baseline 2 exhaustive search：`include/rsp/exhaustive_search.hpp`, `src/exhaustive_search.cpp`
- 实验四中的 robust VI policy 和对比数据

csy：

- 剩余核心内容：Policy Iteration、proper policy、IO、runner、批量实验脚本
- 实验一 toy example 汇总
- 实验三中规模效率比较
- 报告、可视化和最终整合

所有成员新增算法或 baseline 后，都要在 `src/runner.cpp` 注册算法名，并同步更新 `README.md` 与 `docs/API.md`。
