# 《鲁棒最短路(RSP)算法复现》实验报告 — 写作大纲与构思

> 目标:70–80 页 docx。复现论文 **D. P. Bertsekas, "Robust Shortest Path Planning and Semicontractive Dynamic Programming", Naval Research Logistics 66:15–37, 2019**。
> 仓库:`Luchutong/RobustShortestPath`(`red` 分支)。
> 本大纲给出:章节结构 + 每章页数预算 + 对应论文公式 + 对应源码 + 图表 + 优化方法 + docx 排版 + 写作策略。

---

## 0. 总体结构与页数预算

报告分 8 个部分 + 附录,总计约 **78 页正文**(可在 70–80 间伸缩)。"重量"主要落在 **第二部分(理论解读)**、**第四部分(源码精讲)**、**第六部分(实验分析)** 三块——这三块撑起约 55 页,是避免"凑页数"的关键。

| 部分 | 章 | 内容 | 页数 |
| --- | --- | --- | ---: |
| 一 引言与背景 | 1–2 | 绪论、文献综述 | 9 |
| 二 论文解读与理论 | 3–6 | 建模、minimax/Bellman、半压缩 DP、RSP 分析 | 23 |
| 三 算法 | 7–10 | VI / PI / Dijkstra-like / Rollout | 13 |
| 四 复现实现(源码精讲) | 11–19 | 架构 + 9 个模块逐一精讲 | 17 |
| 五 数据集构造 | 20–22 | 格式、layered DAG 生成器、陷阱图族 | 7 |
| 六 实验与结果分析 | 23–28 | 设置 + 实验1/3/4/4b + 正确性验证 | 14 |
| 七 优化与改进 | 29–32 | 算法/工程/方法学/理论延伸 | 9 |
| 八 结论 | 33–34 | 总结、展望 | 4 |
| 附录 | A–D | 公式对照、源码清单、数据表、复现命令、参考文献 | 7 |
| | | **合计** | **≈ 78** |

> 调节到 70 页:压缩第二部分证明细节(只保留命题陈述 + 证明提纲)与附录 B 源码清单。
> 扩到 80 页:第四部分每个模块多放代码片段与时序图,第六部分多放图。

---

## 第一部分 引言与背景(≈ 9 页)

### 第 1 章 绪论(≈ 5 页)
- **1.1 问题背景**:鲁棒/最坏情形最短路的动机。论文 §1 列出的应用——运动规划、追逃(pursuit-evasion)、模型预测控制(MPC)、Markov 决策、序贯博弈、鲁棒组合优化、HJB/eikonal 方程离散化。强调"对手在每个节点从 `Y(x,u)` 选最坏后继"这一 set-membership 不确定性。
- **1.2 与经典/随机最短路的区别**:经典 SP(确定后继)→ 随机 SP(SSP,概率后继)→ **RSP(对抗后继,minimax)**。强调 RSP 只在 **proper policy** 上最小化(必须保证到达终点),且 min-max ≠ max-min(对手知道我们的决策)。
- **1.3 本次大作业任务**:复现该算法体系(VI / PI / Dijkstra-like / proper policy / rollout / baseline 对照),构造数据集,做三组实验,并给出工程与方法学改进。
- **1.4 小组分工**:hhm(Dijkstra-like + deterministic baselines + 实验4)、lct(VI + exhaustive + 实验3)、csy(PI + proper policy + IO + 实验1 + 整合)。引用 README 的分工表。
- **1.5 报告组织**:本报告章节导航。

### 第 2 章 文献综述(≈ 4 页)
- **2.1 经典最短路**:Dijkstra、Bellman-Ford、label-setting/label-correcting([2,28,48])。
- **2.2 随机最短路(SSP)**:Bertsekas-Tsitsiklis 1991([19])、proper policy 概念、收敛性。
- **2.3 鲁棒优化与博弈**:鲁棒优化([5,9])、序贯博弈与 minimax([11,46])、追逃([47,68])、鲁棒最短路 RSP 既有工作([17,63,87])。
- **2.4 半压缩 DP**:抽象 DP([16,25,30,31]),特别是 [31] *Abstract Dynamic Programming* 的半压缩模型——本文理论根基。
- **2.5 相关 Dijkstra-type 博弈算法**:Bardi-Lopez([12])——与本文 Dijkstra-like 最接近、但收敛性分析更弱。
- 用一段说明"本文新意":把 RSP 嵌入半压缩抽象 DP,允许非负**和负**边权,给出有限终止的 Dijkstra-like。

---

## 第二部分 论文解读与理论(≈ 23 页,报告"理论重量"所在)

> 写法:每个公式都"陈述 + 直觉解释 + 与项目实现的对应"。命题给"陈述 + 证明提纲",关键的(Prop 4.3、有限终止)给较完整证明。配论文 Fig 1/2/3。

### 第 3 章 RSP 问题建模(≈ 6 页)
- **3.1 图与策略**:节点集 `X∪{t}`、弧集 `A`、控制集 `U(x)`、不确定后继集 `Y(x,u)⊂X∪{t}`、代价 `g(x,u,y)`;终点 `t` 吸收且零代价。策略 `μ: x↦μ(x)∈U(x)`。
- **3.2 路径与策略代价**:路径 `p={(x₀,x₁),(x₁,x₂),…}`、路径长度 `L_μ(p)=Σg`(式 1.1)、`P(x,μ)`。
- **3.3 proper / improper**:弧子图 `A_μ=∪_x{(x,y)|y∈Y(x,μ(x))}`;**proper ⟺ `A_μ` 无环**(所有路径有限步终止);improper ⟺ 含环。配 **Fig 1**(同一图的 improper μ 与 proper μ̄)。
- **3.4 策略的最坏代价**:`J_μ(x)=max_{p∈P(x,μ)} L_μ(p)`(式 1.2/2.1)= proper μ 下从 x 到 t 的**最长路**;improper μ 用 `limsup`(式 2.2)。**这是项目 `evaluate_proper_policy_dag` 的数学定义**。
- **3.5 RSP 问题与 Assumption 2.1**:`J*(x)=min_{μ proper} J_μ(x)`,在所有 proper 策略上**同时**对每个 x 最小化(式 2.5 是在所有策略上,见 §4)。**Assumption 2.1**:(a) 至少存在一个 proper 策略;(b) 每个 improper 策略的所有环长为正。解释这对应项目里"layered DAG 保证 proper 存在 + 正代价"。

### 第 4 章 Minimax 形式与 Bellman 方程(≈ 5 页)
- **4.1 Bellman 方程**:`J_μ(x)=max_{y∈Y(x,μ(x))}[g(x,μ(x),y)+J̃_μ(y)]`(式 2.3),`J̃` 在 `t` 处取 0(式 2.4)。**这是项目 `action_value` 的核心**。
- **4.2 三个算子**:
  - `H(x,u,J)=max_{y∈Y(x,u)}[g(x,u,y)+J̃(y)]`(式 2.6)→ 项目 `action_value(graph,x,a,J)`。
  - `(T_μ J)(x)=H(x,μ(x),J)`(式 2.8)→ 固定策略求值。
  - `(T J)(x)=min_{u∈U(x)} H(x,u,J)`(式 2.9/2.10)→ 项目 `bellman_update` + `greedy_action`。
- **4.3 最优性**:`J*=min_μ J_μ`(式 2.5);主结果——`J*` 是 `T` 在 `R(X)` 上唯一不动点,`T^k J→J*`,存在最优 proper 策略,μ 最优 ⟺ 在式 2.10 取到 min。
- **4.4 min-max ≠ max-min**:对手知道我方决策,故"先 min 后 max"。强调这点对建模/实现的影响。

### 第 5 章 半压缩动态规划理论(≈ 6 页)
- **5.1 抽象 DP 模型**:状态 `X`、控制 `U`、单调算子 `T_μ`(`J≤J'⇒T_μJ≤T_μJ'`)、`T=inf_μ T_μ`、`J_μ=limsup T_μ^k J̄`。
- **5.2 S-regularity**(Def 3.1):μ 是 S-regular ⟺ `J_μ∈S` 且 `∀J∈S, T_μ^k J→J_μ`。直觉:`J_μ` 是 `T_μ` 在 `S` 上的渐近稳定不动点(类压缩)。**proper 策略 = R(X)-regular**(Prop 4.2)。
- **5.3 Assumption 3.1 与 Prop 3.1/3.2**:S-irregular 策略代价为 ∞(不可能最优);`J*` 是 `T` 在 `S` 上唯一不动点,存在最优 S-regular 策略。
- **5.4 为什么"半"压缩**:只有部分策略(proper)有压缩性质,improper 没有 → 介于压缩 DP 与单调 DP 之间。给出与经典 SSP 的类比。

### 第 6 章 RSP 的半压缩分析与主结果(≈ 6 页)
- **6.1 Prop 4.1**:improper μ 的 `J_μ` 按环长符号分类(全非正→有限上界;全非负→有限下界;全零→实值;有正环→某状态 ∞)。**直接解释项目里 improper baseline 为何 worst_cost=∞**。
- **6.2 Prop 4.2(关键刻画)**:μ 是 R(X)-regular ⟺ `A_μ` destination-connected 且所有环非负 ⟺ μ proper,或 improper 但所有环负。**对应 `check_policy_proper`**。
- **6.3 Prop 4.3(主定理)**:Assumption 2.1 下,`J*` 唯一不动点、`T^k J→J*`、存在最优 proper 策略、最优性刻画。给较完整证明提纲(嵌入 Prop 3.1)。
- **6.4 反例与边界情形**:Example 4.1(单节点自环,Fig 2)、Example 4.2(improper 反而最优,Fig 3)——说明 Assumption 2.1(b) 不可少;§4.1 负环(Assumption 4.1/4.2)、§4.2 零环(Assumption 4.3)+ 扰动法(Prop 4.5)。**项目用非负代价 + 正环对应 Assumption 2.1/4.3,是最稳妥设定**。

---

## 第三部分 算法(≈ 13 页)

> 每个算法:数学描述 + **伪代码** + 复杂度 + 收敛性 + 论文示例(Fig 4/5)+ 指向第四部分的实现。

### 第 7 章 值迭代 VI(≈ 4 页)
- 迭代 `J_{k+1}=T J_k`,从 `J₀≡∞` 出发(§5.1)。
- **有限终止**:Assumption 2.1 下 ≤ N 步收敛(类 Bellman-Ford);集合 `X_k`(式 5.1)刻画"第 k 步已确定哪些节点的 `J*`"(式 5.2)。
- **all-nodes-at-once vs one-node-at-a-time(Gauss-Seidel)**:Fig 4 例子,后者每轮算量约为前者的 1/N。
- 伪代码 + 复杂度 + 指向 `value_iteration.cpp`。

### 第 8 章 策略迭代 PI(≈ 4 页)
- 从 proper `μ₀` 起;求值(DAG 最长路)→ 改进 `μ_{k+1}` 使 `T_{μ_{k+1}}J_{μ_k}=T J_{μ_k}`;`J_{μ_k}` 单调降,有限步到 `J*`(式 5.5)。
- **乐观/异步 PI**:引入阈值函数 `V`(式 5.6/5.7)以容忍 improper 中间策略;one-node Gauss-Seidel PI。
- 伪代码 + 指向 `policy_iteration.cpp`(注意项目的"严格改进 + 优雅失败"实现)。

### 第 9 章 Dijkstra-like 算法(≈ 4 页)
- **Assumption 5.1**(proper 存在 + improper 正环 + **非负边权**)。
- 维护候选集 `V`、永久集 `W`、标号 `J`:初始 `V={t},J(t)=0,J(x)=∞`;每轮取 `y*=argmin_{y∈V}J(y)` 永久化,对 `x∉W` 当 `Y(x,u)⊂W∪{y*}` 时更新 `J(x)=min_u max_y[g+J(y)]`(式 5.8–5.12,`Û(x)`)。
- **N+1 步终止**(Prop 5.2)、**正确性 `J=J*`**(Prop 5.3)、复杂度 `O(NAL)`;Fig 5 示例(节点出 V 顺序 1,4,3,2)。
- 指向 `dijkstra_like.cpp`(项目的"所有后继 finalized 才成候选"= `Y(x,u)⊂W`)。

### 第 10 章 Rollout 近似(≈ 1 页)
- base policy μ(proper)→ rollout policy `μ̄(x)=argmin_u max_y[g+J̃_μ(y)]`(式 5.19,= 单步 PI);保证 `J_μ̄≤J_μ`。指向 `adversarial_rollout` 与实验4。

---

## 第四部分 复现实现 / 源码精讲(≈ 17 页,报告"工程重量"所在)

> 写法:每个模块给 **接口签名 + 关键代码片段(10–25 行) + 逐行/逐块讲解 + 与论文公式的对应**。代码用等宽字体、带行号、配简短注释。

### 第 11 章 系统架构与工程体系(≈ 2 页)
- 仓库结构(`include/rsp/`、`src/`、`experiments/`、`tests/`、`visualization/`、`data/`、`report/`、`docs/`、CI)。
- 统一接口设计:`graph.hpp`(数据)、`bellman.hpp`(算子)、`runner.hpp`(统一运行)、`io.hpp`、`proper_policy.hpp`。一张"数据流图"(读图 → run_algorithm → CSV → 可视化)。
- 构建(CMake,默认 Release、Sanitizer、`RSP_WERROR`)、CI(ubuntu+macOS 矩阵、strict-warnings、smoke)。

### 第 12 章 核心数据结构与数值工具(≈ 1.5 页)
- `Transition{to,cost}` / `Action{action_id,name,trans}` / `Node{id,actions}` / `RobustGraph{n,terminal,nodes}`。映射论文 `(x,u,y)、g、U(x)、Y(x,u)`。
- `utils.hpp`:`INF=1e100`、`EPS=1e-9`、`safe_add`、`is_inf`、`finite_abs_diff`、`less_with_eps`。讲为何用 INF 哨兵表示"不可达/improper 的 ∞"(对应 Prop 4.1)。

### 第 13 章 Bellman 算子实现(`bellman.cpp`)(≈ 1.5 页)
- `action_value`(`max_y[g+J]` ← 式 2.6)、`greedy_action`(`argmin_u` ← 式 2.10)、`bellman_update`(`T J`)、`extract_greedy_policy`。逐一对应 H/T/T_μ。

### 第 14 章 Proper Policy 实现(`proper_policy.cpp`)(≈ 2.5 页)
- `find_initial_proper_policy`:对抗语义下的反向可达分层(对应论文式 5.1 的 `N_k`/`X_k` 思想)——"某 action 的**所有**后继都已知才算可达"。
- `check_policy_proper`:Kahn 拓扑判环(`A_μ` 无环 ⟺ proper)+ 反向可达终点。**对应 Prop 4.2**。
- `evaluate_proper_policy_dag`:逆拓扑序回代 `max_y[g+J]`(= DAG 最长路 = `J_μ`,式 1.2/2.1)。

### 第 15 章 值迭代实现(`value_iteration.cpp`)(≈ 2 页)
- `J₀=∞` 初始化、残差 `finite_abs_diff`、收敛判据、有限终止、`all_values_finite`(不可达节点保持 INF)、抽取贪心策略。对应 §5.1 + Prop 4.3。

### 第 16 章 策略迭代实现(`policy_iteration.cpp`)(≈ 2 页)
- 初始 proper → DAG 求值 → 严格改进(`less_with_eps`)→ 收敛/优雅失败。对应 §5.2、式 5.5。讲项目把"改进后变 improper / 无初始 proper"改为优雅返回 `success=false`(与理论 Prop 4.3 一致:这些情形不会最优)。

### 第 17 章 Dijkstra-like 实现(`dijkstra_like.cpp`)(≈ 2 页)
- finalized 集 = `W`;"action 全部后继已 finalized"= `Y(x,u)⊂W`;取最小候选 finalize;tie-break(node id、action index)保证确定性;`success/failure`。对应 §5.3、Prop 5.1–5.3。给非负代价下"取最小候选可永久化"的正确性论证(项目证明已写入 `report/proof_section.md` §5)。

### 第 18 章 Baseline、Exhaustive 与 Rollout(`baseline.cpp` / `exhaustive_search.cpp`)(≈ 2 页)
- 三种确定化 baseline(nominal=首后继 / bestcase=全后继边 / worst_immediate=最坏即时代价)+ 反向 Dijkstra → 再用鲁棒求值复核(对应"忽略对手"的对照)。
- `adversarial_rollout`:对手每步取 `max[g+J]`(式 5.19 的 rollout 思想 + 最坏后继)。
- `exhaustive_search`:枚举所有 proper 策略取逐点最小 = ground truth `J*`(对应"min over proper policies");规模守卫。

### 第 19 章 IO、Runner 与 CLI(`io.cpp` / `runner.cpp` / `main.cpp`)(≈ 1.5 页)
- `read_graph_txt`(.txt 文法解析 + 校验:非负代价、`<INF/2`、action_id 唯一、尾数据拒绝)、CSV/JSON 输出(`format_value` 的 `inf` 哨兵、RFC4180 转义)、`run_algorithm` 统一派发、`main` 参数解析与退出码。

---

## 第五部分 数据集构造(≈ 7 页)

### 第 20 章 数据格式与 Toy 图(≈ 2 页)
- `.txt` 文法(`n terminal` + 每节点 action 块)与 `.json`(可视化)。`docs/INPUT_OUTPUT.md`。
- **Toy 图**(`data/toy_graph.txt`,6 节点):手算 `J*=[7,1,101,4,100,0]`。讲清"节点0 action0 鲁棒代价 `max(1+J₁,1+J₂)=102`,action1 `=3+J₃=7`"——正是论文 `J_μ=max path` 的最小实例。配图(`toy_graph.svg`)。

### 第 21 章 中规模随机图生成器(`generate_medium_graphs.py`)(≈ 3 页)
- **layered DAG** 设计:`build_layers`(分层)、`later_nodes`(只连后续层或终点)、`choose_successors`(去重、强制 action0 指向更后的"safe"后继)。**为何这样设计**:天然无环 + 每个非终点至少一条 safe action 到终点 ⟹ **保证存在 proper policy + 非负代价 ⟹ 满足 Assumption 2.1/5.1**(把理论假设落到数据生成上,是本章亮点)。
- `rng_seed = seed·1_000_003 + n·9_176 + s·53 + case`(含 `s`,跨 s 独立采样);`graph_metadata.csv` 13 列 schema;文件名规范。
- 已证明的保证:层非空递增、safe 链必达终点、后继集非空(报告可引用我们的形式化论证)。

### 第 22 章 陷阱图族(`generate_trap_graphs.py`)(≈ 2 页)
- 3 节点 gadget:风险 action 的 nominal 后继便宜、对抗后继成环;另有 safe action。**存在 proper 策略,但 nominal/bestcase baseline 被诱导选入对手下成环的 improper 策略**。
- 直接呼应理论:这是 **Prop 4.1/4.2** 的实例化——improper 策略含正环 ⟹ `J_μ=∞`。是"数据集服务于理论验证"的范例。

---

## 第六部分 实验与结果分析(≈ 14 页,报告"结果重量"所在)

### 第 23 章 实验设置与指标(≈ 1.5 页)
- 环境(Ubuntu/g++12/CMake;Release)、构建、复现命令。指标定义:`runtime_ms / iterations / success / success_rate / avg_value / worst_cost / valid_rate / terminated_rate`。

### 第 24 章 实验 1:Toy Example(≈ 2.5 页)
- vi/pi/dijkstra/exhaustive 在 toy 上 `J*=[7,1,101,4,100,0]` 完全一致(表 + `toy_values.svg`)。
- 鲁棒 vs 名义:`baseline_nominal` rollout worst_cost=102,vi=7(表 + `toy_steps.svg`)。
- 解读:RSP 求的是"对抗最坏后继下最优的 policy",而非单条最短路。呼应论文 `J_μ=max path`。

### 第 25 章 实验 3:中规模效率比较(≈ 3 页)
- vi/pi/dijkstra 在 n=20/50/100/200(每规模 20 图)的 runtime/iterations/success/avg_value(完整表 + `runtime_comparison.svg`)。
- **关键解读(连理论)**:三者 `avg_value` 完全一致 ⟹ 都求得同一 `J*`;dijkstra 的 `iterations=节点数`(对应 Prop 5.2 的 N+1 终止);vi 迭代数少(对应 §5.1 有限终止 ≤N);`vi` runtime 最小(n≥50),`pi/dijkstra` 同量级(亚毫秒、噪声敏感,需诚实说明)。

### 第 26 章 实验 4:鲁棒性对比(≈ 2.5 页)
- baselines vs vi 在 s=1..5(n=50,每 s 20 图)的 worst_cost(矩阵表 + 图)。s=1 全相同(无不确定性),s≥2 vi 最低。
- 方法学诚实:`avg_worst_cost` 只对 valid&terminated 求平均(幸存者偏差),需明确;layered DAG 使所有 policy 恒 proper(`valid_rate=1.0` 是构造产物)。

### 第 27 章 实验 4b:baseline 失效 + 配对统计 + 多种子(≈ 3 页)
- **陷阱图**:nominal/bestcase `valid_rate=0.15`、worst_immediate=0.50、vi=1.00 —— baseline 真正失效(呼应 Prop 4.1)。
- **配对统计**:实验4 逐图 vi 对每个 baseline 100/100 不劣(0/300 落败),比"均值更低"强。
- **多种子**(42/123/2024):vi 配对胜率累计 900/900;s=5 worst_cost `66.97±0.75` vs baseline ~93–102。结论不依赖随机种子。

### 第 28 章 复现保真度与正确性验证(≈ 1.5 页)
- ~28 万张随机图上 vi=pi=dijkstra=exhaustive 零不匹配(对应 `J*` 唯一性,Prop 4.3);Sanitizer 干净;多智能体审计。这是"复现忠实于论文定理"的有力证据。

---

## 第七部分 优化与改进(≈ 9 页 —— 你要求的"更好的优化方法")

### 第 29 章 算法层面(≈ 4 页)
1. **Dijkstra-like 用堆**:现实现是 `O(n²·A·S)` 扫描式;论文给 `O(NAL)`,可进一步用二叉堆/Fibonacci 堆做 `argmin J` 选择 → 近 `O((E+N)logN)`。讨论"所有后继 finalized"约束下堆的维护难点。
2. **Gauss-Seidel / one-node VI**(论文 §5.1):按 `X_k` 顺序逐节点更新,迭代数与算量都更省;给出与 all-nodes 的对比预期(论文 Fig 4 显示 1/N 算量)。
3. **乐观/异步 PI + 阈值 V**(论文 §5.2):容忍 improper 中间策略,适合分布式/无初始 proper 策略场景。
4. **Rollout 在线近似**(论文 §5.4):大图上不求全局 `J*`,用 base policy 单步改进,`J_μ̄≤J_μ`,适合实时/追逃。
5. **VI 微优化**:`bellman_update` 单遍求 min(项目已做)、`value_history` 默认关闭(已做)。

### 第 30 章 工程层面(≈ 2.5 页)
- 健壮性:exhaustive 规模守卫、读图尾数据/n 上限/INF 代价校验、graceful failure 统一、CSV 转义(项目已落实,可作为"复现工程化"成果展示)。
- 构建/CI:默认 Release、`-Werror` 严格 job、OS/编译器矩阵、smoke 数值断言。
- 测试:300 随机图跨算法一致性、rollout 超时、exhaustive 守卫、PI 改进历史等。

### 第 31 章 方法学层面(≈ 1.5 页)
- 陷阱图族(展示 baseline 失效)、配对统计、多种子方差/置信区间——已落实,可进一步:负代价图族(对应 Assumption 4.1 的负环 longest-path)、零环图族(Assumption 4.3)、bootstrap 置信区间、更大规模(需先做堆优化)。

### 第 32 章 理论延伸(≈ 1 页)
- 论文 §6 的**随机扩展**(式 6.1:`H` 加概率 `p_xz`,RSP+SSP 混合)、负边权 longest-path、不完美状态信息(set-membership estimator)——作为未来方向,说明项目当前聚焦非负代价的 Assumption 2.1/5.1 子集。

---

## 第八部分 结论(≈ 4 页)

### 第 33 章 总结(≈ 2 页)
- 复现了论文 VI/PI/Dijkstra-like/proper-policy/rollout 全套;数值与理论一致(`J*` 唯一、有限终止、proper 最优);三组实验 + 陷阱图 + 配对/多种子统计验证了鲁棒规划优势;并做了系统的工程与方法学加固。

### 第 34 章 不足与展望(≈ 2 页)
- 当前仅非负代价 + layered/陷阱图族;Dijkstra-like 为 naive 实现;未覆盖负环/零环/随机扩展。展望:堆优化、Gauss-Seidel、随机扩展、更大规模、真实地图数据(追逃)。

---

## 附录(≈ 7 页)
- **附录 A 论文公式 ↔ 代码 速查表**(见下方,扩成整页表格)。
- **附录 B 关键源码清单**(VI/PI/Dijkstra-like/proper_policy 核心函数)。
- **附录 C 完整实验数据表**(exp3 全表、exp4 全 s×policy 矩阵、exp4b、多种子)。
- **附录 D 复现命令与环境**。
- **参考文献**:论文 [Bertsekas 2019] + 选引论文 88 篇中的核心([12,19,30,31] 等)+ 本项目仓库。

---

## 论文 ↔ 代码 ↔ 报告 速查映射(写作时对照)

| 论文 | 公式/命题 | 项目代码 | 报告章 |
| --- | --- | --- | --- |
| 后继代价算子 H | 式 2.6 | `bellman.cpp::action_value` | 4.2 / 13 |
| 固定策略算子 T_μ | 式 2.8 | `action_value`(固定 a) | 13 |
| 最优算子 T | 式 2.9/2.10 | `bellman_update`+`greedy_action` | 13 / 15 |
| `J_μ`=最长路 | 式 1.2/2.1 | `evaluate_proper_policy_dag` | 3.4 / 14 |
| proper ⟺ 无环 | §1.2 / Prop 4.2 | `check_policy_proper`(Kahn) | 3.3 / 14 |
| 初始 proper(反向可达) | 式 5.1 `N_k`/`X_k` | `find_initial_proper_policy` | 14 |
| VI 有限终止 | §5.1 / Prop 4.3 | `value_iteration.cpp` | 7 / 15 |
| PI 单调收敛 | 式 5.5 | `policy_iteration.cpp` | 8 / 16 |
| Dijkstra-like N+1 终止 | 式 5.8–5.12 / Prop 5.2/5.3 | `dijkstra_like.cpp` | 9 / 17 |
| improper→∞ | Prop 4.1 | improper baseline `worst_cost=inf` | 6.1 / 22 / 27 |
| Rollout 单步改进 | 式 5.19 | `adversarial_rollout` | 10 / 18 |
| `J*`=min over proper | 式 2.5 | `exhaustive_search` | 18 / 28 |

---

## docx 排版与页数策略

### 版式建议(便于稳定到 70–80 页)
- **正文**:中文宋体 / 英文 Times New Roman,小四(12pt),1.5 倍行距,A4,页边距 2.5cm。
- **标题**:黑体;一级标题三号、二级小三、三级四号;自动多级编号 + 自动目录。
- **公式**:用 Word 公式编辑器,**右侧编号**(2.6)(3.1)…,正文用 `J*`、`T_μ` 等斜体变量。
- **代码**:等宽(Consolas 10.5pt)、浅灰底纹、带行号、≤25 行/段,配 figure 题注"代码 X-Y:…"。
- **图**:SVG 转高清 PNG 插入,统一题注"图 X-Y:…",正文引用"如图 X-Y";项目已有 `toy_graph/toy_values/toy_policy_vi/toy_steps/runtime_comparison`.svg。
- **表**:三线表,题注在表上方"表 X-Y:…"。
- **交叉引用**:图/表/公式/章节全部用 Word 交叉引用(改动不乱号)。

### 页数从哪里来(避免"凑")
- 理论(第二部分 23 页):每个公式"陈述+直觉+对应实现"+命题证明提纲,自然占满。
- 源码(第四部分 17 页):9 个模块 × 平均 ~2 页(接口+片段+讲解+论文对应)。
- 实验(第六部分 14 页):4 组实验 × 完整表 + 图 + 解读。
- 三块合计 ≈ 54 页是"硬内容";加引言/算法/数据/优化/结论/附录 ≈ 24 页 → 78 页。
- **不要**靠放大字号/行距/截图凑页;靠"公式逐条解读 + 代码逐块讲 + 实验逐图析 + 论文↔代码逐项对应"。

### 建议写作顺序(降低返工)
1. 先写第四部分(源码,你最熟)→ 2. 第六部分(实验,数据已齐)→ 3. 第五部分(数据集)→ 4. 第二/三部分(理论/算法,边写边对照本大纲的公式)→ 5. 第七部分(优化)→ 6. 第一/八部分(引言/结论)→ 7. 附录与目录/交叉引用统一。
