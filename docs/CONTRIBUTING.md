# 协作规范

## 分支建议

```text
main
feature/vi
feature/pi
feature/dijkstra
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

## 推荐任务拆分

成员 A：

- 完成 `report/theory_notes.md`
- 完成 `report/proof_section.md`
- 补充 README 中的问题定义和算法说明

成员 B：

- 强化 `policy_iteration`
- 实现 optimized Dijkstra-like
- 完成更多单元测试
- 实现 deterministic Dijkstra baselines

成员 C：

- 实现 layered random graph generator
- 编写批量实验脚本
- 生成 correctness/runtime/robustness 图表
- 整理报告图片

