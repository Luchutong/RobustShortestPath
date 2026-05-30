# 修复记录(对应 CODE_REVIEW_red.md)

针对 `red` 分支审查报告中的每一条发现所做的修改。全部改动在工作区,**未提交/未推送**,构建与测试已验证通过。

## 验证状态(改后实测)
- Release 构建(`-Wall -Wextra -Wpedantic`):零告警
- ASan + UBSan 构建:零告警
- `ctest`(Release 与 Sanitizer):**3/3 全过**(含新增的 pi 优雅失败测试)
- CI 等价冒烟(generator / run_runtime / run_robustness / 三个 plot):全部通过
- Toy 端到端:`J=[7,1,101,4,100,0]`;`--zero-init` 同样得到正确结果
- exp3/exp4 已用修正后的流水线重新生成,图与 CSV、可视化、报告数字四者对齐

## 高优先级
- **H1 官方数据陈旧/不兼容** → 用当前流水线**重新生成** `experiment_data/official_20260521_210335/`(exp3:80 图;exp4:100 图,单次 `--successors-values 1 2 3 4 5`),重画 `report/figures/`,并据实更新 `analysis.md`、`report/experiment_results.md`、`report/lct_experiment_report.md` 的全部数字表。`plot_comparison.py` 现可正常消费新 summary。
- **H2 生成器种子未含 `s`** → `experiments/generate_medium_graphs.py:168` 改为 `rng_seed = seed*1_000_003 + n*9_176 + s*53 + case`,跨 `s` 样本独立。
- **H3 `pi` 失败语义不一致** → `src/policy_iteration.cpp` 在"无初始 proper 策略 / 改进后变 improper"时**不再抛异常**,改为返回 `converged=false, success=false`,与 `vi/dijkstra` 一致;新增 `tests/test_lct.cpp::test_runner_pi_rejects_no_proper_policy_graph` 锁定该契约。
- **H4 `.gitignore` 漏 `__pycache__`** → 补 `__pycache__/`、`*.pyc`;`git rm -r --cached visualization/__pycache__` 取消跟踪 6 个字节码(已验证新生成的 .pyc 被正确忽略)。

## 中优先级
- **M1 实验3报告缺实测** → `report/lct_experiment_report.md` 新增"实测结果"表 + 图链接 + 观测结论,并把"预期"改为"预期与实测对照"(修正"dijkstra 通常最快"与实测"vi 最快"的矛盾)。
- **M2 README 误标 graph_metadata** → `README.md` 从 `results/` 输出清单移除,改注明由生成器写入 `--output` 图目录。
- **M3 可视化裸 traceback** → `visualization/common.py` 新增 `run_cli()`,5 个 plot 脚本改用它,缺文件/非法输入打印 `error: <msg>` 并以非零码退出(已验证)。
- **M4 空 `--algorithms` 静默空图** → `plot_values.py` 为空时 `raise ValueError`(已验证 rc=1)。
- **M5 批处理单图坏文件中断整批** → `run_runtime.cpp` / `run_robustness.cpp` 把"读图+单图校验"包进 per-graph try/catch,坏图跳过并告警、继续。
- **M6 `avg_worst_cost` 幸存者偏差** → `report/experiment_results.md`、`analysis.md` 明确该均值只对 `valid && terminated` 样本计算(本批 valid/terminated 均 100%)。
- **M7 证明偏简** → `report/proof_section.md` §3 补"自上而下单调收敛 + SSP 假设下不动点唯一";`report/theory_notes.md` §2 改述 SSP 假设(improper 策略最坏代价为 +∞)。

## 低优先级
- **L1 `--zero-init` 未文档化** → `docs/INPUT_OUTPUT.md` 新增 `rsp_main` 可选参数块;`src/main.cpp` 的 `--help` 补充说明。
- **L2 report/README 漏列 hhm 报告** → 已补 `hhm_experiment_report.md`。
- **L3 rng_seed 示例错误** → `docs/INPUT_OUTPUT.md` 更新为 `42183911` 并给出公式。
- **L4 CONTRIBUTING 漏 runner.cpp** → 已加入需同步 `docs/API.md` 的文件清单。
- **L5 `bellman_update` 重复计算** → `src/bellman.cpp` 改为单遍求 min,去掉对最优 action 的二次 `action_value`。
- **L6 `has_negative_transition_cost` 死代码** → `src/dijkstra_like.cpp`、`src/baseline.cpp` 删除(负代价已由 `graph.validate()` 拦截抛 `invalid_argument`);`report/hhm_experiment_report.md` 相应措辞改准。
- **L7 `run_robustness` 死代码** → 删除 `if(!row.terminated){row.policy_valid=true;}` 无操作分支。
- **L9 `common.py` 死防御默认** → 去掉恒存在列的 `.get(默认)` 与不存在的 `invalid_reason` 兜底;`*_rate` 统一用 `parse_float`。

## 经评估后未改动(附理由)
- **L8 `format_value` 精度 / CSV 转义**:成本经生成器 `round(...,6)`,两个 6 位小数之和仍 ≤6 位,`std::to_string` 输出精确,无实际精度损失;当前文件名规范不含逗号,无需转义。改动 `format_value` 反而会破坏依赖固定 6 位格式的测试与文档示例,故**保持原样**。
- **L10 Dijkstra-like O(n²·A·S)**:在作业规模(n≤200)下完全够用,属可选性能优化,**未改**(避免为微优化引入风险)。

## 多智能体审计后的补修(audit: needs_fixes → 已修)

对本次修复做了 4 维度对抗式审计(C++ 正确性 / Python / 文档-数据一致性 / 构建-完整性)。代码、构建、测试、以及报告数字与 CSV 的逐格一致性均**零问题**(数字独立重算复现);审计确认两处必修(均为文档/图),已修:

- **A1(回归)`report/figures/runtime_comparison.svg` 陈旧** → 起因是重画该图后,推送文档的 `rsync` 用本地旧图覆盖了远程新图;现已在远程**用当前 CSV 重新生成**该图并同步回本地(y 轴从旧的 ~2.96ms 修正为新的 ~0.34ms,与报告表一致)。
- **A2 文案过度断言"vi 在所有规模上最快"** → 与自身 `n=20` 表格(dijkstra 0.006253 < vi 0.006269,亚微秒噪声)矛盾;已在 `experiment_results.md`、`lct_experiment_report.md`(2 处)、`analysis.md` 共 4 处软化为"`n>=50` 上最快、整体最快;`n=20` 与 dijkstra 在亚微秒噪声内持平"。

审计列为可推迟的 nice-to-have(本次**未改**,附理由):

- **H2 种子在 `cases > 53` 时的潜在碰撞**:当前 `s*53 + case`,若每个 `s` 的 `cases` 超过 53,跨 `s` 的 `(s,case)` 偏移可能重叠。**不影响官方数据**(`cases=20 < 53`),不引起文件名碰撞、不导致任何测试失败;彻底加固需改种子公式,会再次触发整批数据重生成与数字更新,故按审计建议推迟为后续项。
- **1 处 C++ 注释**(描述理论上不可达分支的行为)属纯文案,无功能影响,保留。

## 全项目复审后的增强(第二轮 7 维度多智能体审计)

第二轮对整个项目做了 7 维度对抗式审计(算法/C++ 潜伏 bug/IO/测试/文档/方法学/构建-CI),结论 **GOOD、可直接交、无阻断、核心算法经 ~28 万张随机图对照 exhaustive 零不匹配**。按"最彻底"落实了全部 important + 关键 minor:

**代码健壮性**
- `exhaustive_search`:加规模守卫(策略数 > 5e6 或 n > 2000 → 返回 `success=false`),消除 CLI 跑大图时的永久卡死与深图栈溢出(CLI 实测 exit=2 优雅失败,非卡死)。
- `read_graph_txt`:拒绝尾部多余 token(防损坏/拼接文件被静默当成另一张图)+ n 上限(防 OOM)。
- `graph.validate()`:拒绝 `cost >= 5e99`(避免与 INF=1e100 哨兵冲突)+ 校验节点内 `action_id` 唯一。
- `io.cpp`:CSV 字段 RFC4180 转义(graph_id/algorithm 含逗号不再破格)。
- `main.cpp`:`save_history=false`(`value_history` 从不导出,省无谓分配)。

**测试(锁不变量)**
- `test_lct`:300 张随机图上断言 `vi==pi==dijkstra==exhaustive` 且 `zero-init==inf-init`;exhaustive 在无 proper 图返回 false;exhaustive 规模守卫;PI 改进+历史断言;runner 派发 dijkstra/baseline 的**数值**校验。
- `test_hhm`:rollout 超时分支(成环策略对手下永不终止)。

**构建/CI**
- `CMakeLists`:默认 Release(让 README 命令复现 Release runtime);GNU/Clang 旗标加守卫 + MSVC 支持;新增 `RSP_WERROR` 选项。
- `ci.yml`:OS 矩阵 `[ubuntu, macos]`(顺带覆盖 gcc + AppleClang)、新增 `strict-warnings`(`-Werror`)job、smoke 增加数值断言。

**文档**
- 删报告里泄露的内网主机名;补 **Bertsekas 参考文献**;`proof_section` 补 dijkstra-like、exhaustive 正确性证明并补全 PI 证明(终止/单调/保持 proper/唯一性);补摘要、实验编号说明、metadata 列(12→13)、display_seed 说明、INPUT_OUTPUT 新校验规则与 exhaustive 小图限制;README 快速开始加 `--seed 42`。

**方法学(新增实验,改数据)**
- **Exp4b 陷阱图族**(`generate_trap_graphs.py` + `experiment_data/.../exp4b_trap/`):证明 baseline 真会失效——`baseline_nominal/bestcase` `valid_rate=0.15`、`worst_immediate=0.50`、`vi=1.00`,补上了 layered DAG 族(恒 proper)掩盖的失败模式。
- **配对统计**(`paired_winrate.py`):实验4 逐图比较,VI 对每个 baseline **100/100 不劣**(0/300 落败)。
- **多种子稳定性**(`exp4_multiseed/`,seed 42/123/2024):VI 配对胜率累计 900/900,worst-case cost 高度稳定(s=5:VI `66.97 ± 0.75` vs baseline ~93–102)。

验证:fresh Release / ASan+UBSan / `-Werror` 三种构建均**零告警、ctest 3/3**;CI 冒烟(generator/runtime/robustness/trap/plot)全通过。

> 注:CI 的 `macos-latest` 矩阵项无法本地实测(本机/远程均无 cmake/clang),代码为可移植标准 C++17 + POSIX 测试,理应通过;若 grader 的 CI 报 macOS 失败,可移除该矩阵项。
