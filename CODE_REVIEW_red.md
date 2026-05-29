# RobustShortestPath —— `red` 分支代码审查 & 大作业整体审查报告

- **审查对象**:`Luchutong/RobustShortestPath` 仓库 `red` 分支(commit `380d611`)
- **审查环境**:远程主机 `remote_gpu_server`(Ubuntu 22.04 / g++ 12.3 / pip-cmake 4.3.2 / Python 3.11),经 SOCKS5 代理克隆
- **审查日期**:2026-05-29
- **审查方式**:通读全部 C++ 源码 + 测试 + 实验驱动 + 构建/CI;远程实际编译(Release + ASan/UBSan)、跑全部测试、端到端复现三个实验;Python 脚本与文档/报告交叉核对

---

## 0. 总体评价(先说结论)

这是一份**质量明显高于一般课程大作业**的工程:统一接口设计清晰、算法实现正确、测试覆盖罕见地充分、文档与代码高度自洽、CI 含 Release/Debug/Sanitizer 三套。**核心算法没有发现正确性 bug**。需要改进的地方集中在两点真问题(实验可复现性的"版本错位"、生成器随机种子相关性)和一批工程/文档层面的打磨项。

### 验证结论(实测,均通过)

| 项目 | 结果 |
| --- | --- |
| Release 构建(`-Wall -Wextra -Wpedantic`) | ✅ **零告警** |
| ASan + UBSan 构建 | ✅ 零告警 |
| `ctest`(Release & Sanitizer) | ✅ **3/3 全过**,sanitizer 无报错 |
| Toy 端到端(vi/pi/dijkstra/exhaustive) | ✅ `J=[7,1,101,4,100,0]` 精确一致;`baseline_nominal` 鲁棒值 102 |
| Toy 鲁棒性 rollout | ✅ 三 baseline `worst_cost=102`,`vi=7`(完美演示鲁棒优势) |
| Exp4 官方图复现 | ✅ `avg_worst_cost`/`avg_steps` 与已提交 summary **逐位吻合**(如 s5-bestcase `113.197262`) |

---

## 1. 重要改进项(High priority)

### H1 ⭐ 已提交的"官方实验数据"是旧版代码产物,与当前代码/文档/可视化错位

这是对一个"**复现实验**"项目最关键的一条。`experiment_data/official_20260521_210335/` 确实在仓库里(数值也能复现),但它是**更早版本的代码生成的快照**,schema 已与当前 `red` 不符:

| 产物 | 已提交(官方快照) | 当前代码产出 |
| --- | --- | --- |
| `exp3 runtime_summary.csv` 表头 | `n,algorithm,cases,...`(**8 列**) | `n,requested_s,actions,algorithm,...`(**10 列**) |
| `exp4 robustness_summary.csv` 表头 | `s,policy_type,cases,terminated_count,...`(**7 列**,无 valid_count/valid_rate) | `...,valid_count,valid_rate,...`(**9 列**) |
| exp3 图文件名 | `medium_n100_seed42.txt`(无 `_s/_a`) | `medium_n100_s{S}_a{A}_case{C}_seed{K}.txt` |

**实测后果**:用当前 `visualization/plot_comparison.py` 喂已提交的官方 exp3 summary 直接崩溃:
```
ValueError: no rows left after runtime filters
```
因为它按 `--requested-s/--actions` 过滤,而旧 8 列 CSV 没有这两列。也就是说**按文档命令无法从已提交官方数据重画对比图**;若重跑实验又会得到列数不同的新 CSV,与报告里引用的旧表不一致。

**修复建议**(任选一致即可):
1. 用当前流水线**重新生成并提交**一份 `official_*` 归档(图 + CSV + 图片),让数据、代码、文档、报告四者对齐;或
2. 报告/`docs` 明确标注官方快照对应的代码版本与 schema,并提供"旧 CSV → 新列"的兼容说明;或
3. 给 `plot_comparison.py` 增加对旧 schema(无 `requested_s/actions`)的兼容回退。

---

### H2 ⭐ 随机图生成器的种子未包含 `s`,跨 `s` 样本不独立

`experiments/generate_medium_graphs.py:168`
```python
rng_seed = args.seed * 1_000_003 + n * 9_176 + case     # 没有 s
```
外层循环是 `for s in successor_values: for n: for case:`(L164–179),`rng_seed` 不含 `s`,所以**同一 `(n, case)` 下不同 `s` 的图共用同一个 `random.Random` 流**,会共享一段公共随机前缀(相同层结构、action0 的强制后继、第一条代价)。实验 4 正是"按 `s` 比较鲁棒性",这种相关性削弱了"每个 `s` 独立采样"的假设。

> 说明:已提交的官方 exp4 图用的是**每个 s 不相交的种子段**(s1→seed520–539、s2→seed620–639……,旧命名),所以官方数据本身没踩这个坑;但按当前文档用 `--successors-values 1 2 5` 一次性生成就会触发。

**修复**(一行):
```python
rng_seed = args.seed * 1_000_003 + n * 9_176 + s * 53 + case
```
改动会改变生成数据,需配合 H1 一并重生成归档。

---

### H3 算法间"无 proper 策略"时的失败语义不一致(`pi` 抛异常,`vi/dijkstra` 优雅降级)

- `vi`(`runner.cpp:52-54`):返回 `success=false`(`converged && proper && finite`)。
- `dijkstra`(`dijkstra_like.cpp:86-89`):无法 finalize 全部节点时 `success=false`。
- `pi`(`policy_iteration.cpp:20-22`):`find_initial_proper_policy` 不存在时**直接 `throw std::runtime_error`**;策略改进若产出 improper 也 `throw`(L70-75)。

批量驱动 `run_runtime.cpp:268-286` 与 `run_robustness.cpp:281-303` **逐图逐算法 try/catch**,所以不会中断整批(已通过 `test_runner_vi_rejects_no_proper_policy_graph` 等覆盖)。但作为库 API,同类失败一个抛异常、两个返回布尔,契约不统一。

**建议**:让 `policy_iteration` 在"无初始 proper 策略 / 改进后变 improper"时返回 `converged=false, final_policy_proper=false`(由 `run_algorithm` 映射为 `success=false`),与 vi/dijkstra 对齐;把异常仅留给"真正的不变量被破坏"。

---

### H4 `.gitignore` 缺 `__pycache__/` 与 `*.pyc`,且已误提交字节码

`visualization/__pycache__/*.cpython-38.pyc` 已被纳入版本管理(`.gitignore` L1-19 未忽略它们),而且是 **Python 3.8 字节码**(远程实际跑 3.11,属过期产物)。

**修复**:
```gitignore
__pycache__/
*.pyc
```
并 `git rm -r --cached visualization/__pycache__` 删除已跟踪的 6 个 `.pyc`。

---

## 2. 建议改进项(Medium priority)

### M1 实验报告:`report/lct_experiment_report.md`(实验3)只有"预期分析",没有实测结果表
该文件到 `## 预期分析`(L58-63)结束,通篇是设计与"通常最快/可能增长"的定性预期,**没有实测 runtime 表、没有图**。实验3的实际数字其实存在(在 `report/experiment_results.md` 与 `experiment_data/.../analysis.md`,且可复现),但作为"lct 实验3"的指定交付件,它读起来像实验前的计划书。
**建议**:把实测的 `runtime_summary` 表 + `runtime_comparison.svg` + 对**观测结果**(而非预期)的分析补进该文件,或显式注明"结果见 experiment_results.md"。

### M2 README 把 `graph_metadata.csv` 误列为 C++ 写到 `results/` 的产物
`README.md:46` 在"输出统一写到"清单里列了 `results/graph_metadata.csv`,但它实际由 **Python 生成器**写到**图输出目录**(如 `data/random_graphs/`,见 `generate_medium_graphs.py:207-208`),`run_runtime`/`main` 都不产出它。
**建议**:从 C++ `results/` 清单移除,归到生成器 `--output` 目录说明里。

### M3 可视化脚本对"缺文件/非法输入"直接抛裸 traceback
`visualization/common.py`(`read_csv_rows` L40、`load_graph_json` L35)无 try 守卫,缺 `--values-csv`/`--graph-json` 时打印 `FileNotFoundError` 栈;`plot_comparison.py` 的 `ValueError`(L38/L96)同样未捕获。这些多是**有意的失败**(README 已说明),但作为 CLI,在 `main()` 里统一 `try/except` 打印 `error: <msg>` 到 stderr(对齐 C++ 的 `main`)更稳。

### M4 `plot_values.py` 空 `--algorithms` 静默产出空图
`plot_values.py:24`:`--algorithms ''` 解析成 `[]`,最终输出近乎空白的 SVG 且 `exit 0`,调用方拿不到任何信号。
**建议**:列表为空时 `raise ValueError("no algorithms requested")`。

### M5 `run_runtime`/`run_robustness` 单个坏文件会中断整批(fail-fast 偏脆)
`run_runtime.cpp:321`(`read_graph_txt`)与 L327-331(文件名 action 标签与结构不符的校验)在 per-graph 循环里但**不在** `run_one_algorithm` 的 try 内;`run_robustness.cpp:335-338`(`--start` 越界)同理。任一图出错会让整批实验返回 1 退出。
**建议**:把"读图 + 单图校验"也包进 per-graph 的 try,记一行错误并 `continue`,使批处理对个别坏图鲁棒。

### M6 `avg_worst_cost` 只对"已终止"样本求平均,存在幸存者偏差
`run_robustness.cpp:224-227, 250-251`:`avg_worst_cost` 仅累加 `terminated && 有限` 的 case。nominal baseline 若产出 improper 策略(被对手卡死)会被排除在均值之外,单看该列可能低估其代价。`valid_rate`/`terminated_rate` 另列已体现失败率,但**报告分析需明确说明该均值是"条件于终止"的**,否则结论易被误读。

### M7 证明/理论笔记的关键步骤偏简略
- `proof_section.md:42-55`:VI 收敛只写"反复应用 T 逼近最小不动点",缺**单调性/有界/不动点唯一性**论证;鉴于默认 `init_with_inf=true`(从 +∞ 单调下降),建议补 Bertsekas 标准的"自上而下单调收敛 + SSP 假设下不动点唯一 = J*"。
- `theory_notes.md:29`:"所有 improper policy 代价可区分"措辞含糊,应改述为标准 SSP 条件——**每个 improper 策略在某状态的最坏代价为 +∞**。

---

## 3. 可选打磨项(Low priority / Nice-to-have)

| # | 位置 | 说明 |
| --- | --- | --- |
| L1 | `main.cpp:61` | `--zero-init` 已实现但 `--help`(L64-67)与所有 `.md` 均未文档化;建议补进 `docs/INPUT_OUTPUT.md` 命令行小节 |
| L2 | `report/README.md:5-8` | 报告索引漏列实际存在的 `hhm_experiment_report.md`(实验4 的主要写作) |
| L3 | `docs/INPUT_OUTPUT.md:123` | `rng_seed` 示例 `42003942` 与公式不符(seed42/n20/case0 实际为 `42*1000003+20*9176+0=42183646`);更新或标注"仅示意" |
| L4 | `docs/CONTRIBUTING.md:32-37` | "改了哪些文件需同步 API.md"清单漏了 `src/runner.cpp`(算法注册点,README:160 与 API:376 都把它视为关键) |
| L5 | `bellman.cpp:83-84` | `bellman_update` 先 `greedy_action`(内部遍历所有 action 算 `action_value`)选出最优 action,再对该 action **重复算一次** `action_value`;可让 `greedy_action` 直接回传其值,省一次计算(微优化) |
| L6 | `dijkstra_like.cpp:11-22`、`baseline.cpp:22-33` | `has_negative_transition_cost` 在 `validate()` 已拒绝负代价后属**不可达分支**(死代码,无害);可移除或加注释说明是防御性冗余 |
| L7 | `run_robustness.cpp:295-297` | early-return 已保证此处 `policy_valid==true`,后面 `if (!row.terminated){ row.policy_valid = true; }` 是无操作的死代码,疑似本意写错,建议删除或修正语义 |
| L8 | `io.cpp:136, utils.hpp:33-35` | CSV 数值用 `std::to_string(double)`(固定 6 位小数,大数值丢精度);如需高精度可改 `ostream` + `setprecision`。另:`graph_id`/`algorithm` 写 CSV 未转义,文件名含逗号会破格(当前命名规范下风险极低) |
| L9 | `common.py:90-104` | 对 `policy_valid/status/valid_count/valid_rate` 用了 `.get(..., 默认)` 兜底,但当前 schema 必然产出这些列、且不存在 `invalid_reason` 列,属误导性死防御;`valid_rate/terminated_rate` 用 `float()` 而非 `parse_float`(理论上可能遇到 `inf`,实际有界安全) |
| L10 | dijkstra-like 复杂度 | 当前 O(n²·A·S) 的扫描式 label-setting,规模(n≤200)完全够用;如追求理论效率可上优先队列(注意"所有后继已 finalize"约束的处理) |

---

## 4. 亮点(值得保留/参考)

- **测试质量**:`test_lct.cpp`(730 行)+ `test_hhm.cpp`(257 行)覆盖了四算法一致性、dijkstra finalize 顺序与死锁失败、三种 baseline 选不同 action、对抗 rollout(102 vs 7)与 tie-break、负代价/越界/非法 CLI/非有限 epsilon、生成器子进程(文件名 `_s` 标签、metadata 13 列 schema、重复参数拒绝)、100 图唯一 ID 等——属专业级。
- **数值与鲁棒性处理**:`utils.hpp` 的 `safe_add/is_inf/finite_abs_diff` 统一处理 INF;`value_iteration` 用 `finite_abs_diff` 计算残差、不可达节点保持 INF 且不阻碍收敛,设计干净。
- **鲁棒 SSP 算法正确性**:`proper_policy`(对抗语义反向分层 + Kahn 判环 + 逆拓扑回代)、把 Dijkstra 推广到 min-max(非负代价下"全后继 finalize 才成候选,取最小 finalize"可证最优)均正确。
- **文档-代码自洽**:`docs/API.md`/`docs/INPUT_OUTPUT.md` 的函数签名、CLI、`.txt` 文法、CSV 列与代码逐一对应,甚至文档化了"文件名不规范则 `requested_s/actions=-1"`的回退。

---

## 5. 建议处理顺序

1. **H1 + H2 一起做**:修种子(H2)→ 用当前流水线重生成并提交官方归档(H1)→ 确认 `plot_comparison.py` 能跑通、报告表格与新 CSV 一致。
2. **H4**:补 `.gitignore`、`git rm --cached` 字节码(一次性,零风险)。
3. **M1**:把实验3实测结果补进 `lct_experiment_report.md`。
4. **H3 + M5 + M6**:统一失败语义、批处理对坏图鲁棒、报告里说明 `avg_worst_cost` 的条件含义。
5. 其余 M/L 项按时间余量打磨。

> 注:本报告基于 `red` 分支独立克隆(`~/RobustShortestPath_red_review`),未改动你已有的 `~/RobustShortestPath`(`hhm` 分支)工作区,也未对 GitHub 远程做任何写操作。
