# 证明与论证提纲

本文件用于整理最终报告中可展开成正式证明的部分。这里先给出可直接扩写的证明结构，而不是写成完全形式化的教科书证明。

## 1. Proper Policy 判定正确性

### 命题

若一个 policy 诱导的非终点子图无环，且每个非终点节点都能沿该 policy 到达 terminal，则该 policy 是 proper policy。

### 证明思路

1. 无环说明任意沿 policy 展开的执行不会在非终点子图中无限循环。
2. 所有节点都可到达 terminal，说明不存在 dead-end component。
3. 因为状态数有限、且不存在非终点环，所以任意轨迹在有限步内进入 terminal。
4. 因而该 policy 满足 properness 定义。

### 反向说明

如果策略图存在非终点环，或者存在节点无法到达 terminal，则对抗者可以让系统永远停留在非终点区域，因此 policy 不是 proper。

## 2. Proper Policy 在 DAG 上的 Evaluation 正确性

### 命题

若给定 policy 是 proper policy，则可以按其诱导 DAG 的逆拓扑顺序精确计算其 worst-case value。

### 证明思路

1. properness 保证非终点策略图无环，因此存在拓扑序。
2. 终点 `t` 的值固定为 `0`。
3. 对任意节点 `x`，其所有后继 `y` 在逆拓扑顺序中都已先被求值。
4. 因此可定义：

```text
J_mu(x) = max_{y in Y(x, mu(x))} [g(x, mu(x), y) + J_mu(y)]
```

5. 该递推唯一确定整张 DAG 上的 value，因此 evaluation 正确。

## 3. Value Iteration 与 Bellman 不动点

### 命题

若图满足实验中的 properness 条件与非负代价设置，则 Value Iteration 收敛到 Bellman 最优值函数 `J*`。

### 证明思路（自上而下单调收敛）

1. **单调性**：`T` 是单调算子——若逐点有 `J ≤ J'`，则 `TJ ≤ TJ'`。因为内层 `max_{y}[g(x,u,y)+J(y)]` 关于 `J` 保序，外层 `min_u` 也保序。
2. **初值是上界**：代码默认 `init_with_inf=true`，即 `J_0(x)=+∞`（terminal 为 0）。于是对任何候选值函数都有 `J_0 ≥ J*`，并且 `T J_0 ≤ J_0`（任何一步更新都不会比 `+∞` 更大）。
3. **单调下降且有下界**：由 `T J_0 ≤ J_0` 与单调性归纳得 `J_{k+1}=T J_k ≤ J_k`，序列逐点单调非增；又因转移代价非负且存在 proper policy，有 `J_k ≥ J* ≥ 0`，序列有下界。
4. **极限是不动点**：单调非增且有下界的实数序列收敛，记逐点极限为 `J_∞`；对算子取极限得 `J_∞ = T J_∞`，即 `J_∞` 满足 Bellman 最优性方程。
5. **不动点唯一 = J\***：在 SSP 假设下（存在 proper policy，且每个 improper policy 在某状态的 worst-case 代价为 `+∞`），Bellman 方程在有限值函数类上有唯一不动点，故 `J_∞ = J*`。
6. **抽取最优策略**：从 `J*` 取 greedy policy（`extract_greedy_policy`）即得 robust optimal policy；该 policy 必为 proper——否则其 worst-case 值为 `+∞`，与 `J*` 在可达状态上有限相矛盾。

注：以上即 Bertsekas 极小化–极大 SSP 的标准单调收敛论证。实现上 `value_iteration` 用 `init_with_inf` 做自上而下迭代，用 `finite_abs_diff` 计算残差，并对仍为 `+∞` 的不可达节点不纳入收敛判据（`all_values_finite` 另行标记）。

## 4. Policy Iteration 的停止条件

### 命题

若在某一轮 policy improvement 后策略不再变化，则当前策略是一个 Bellman 最优策略。

### 证明思路（含改进、保持 proper 与终止性）

1. **评估**：当前 proper 策略 `mu` 的 evaluation（§2 的 DAG 逆拓扑回代）给出其精确值 `J_mu`。
2. **单调改进**：improvement 步骤对每个节点取 `argmin_u H(x,u,J_mu)`。代码采用严格改进（`less_with_eps`），仅当新 action 严格更优才替换，因此新策略 `mu'` 满足 `H(x,mu'(x),J_mu) ≤ H(x,mu(x),J_mu) = J_mu(x)`（逐点不增），且至少在一个状态严格下降，于是 `J_{mu'} ≤ J_mu` 且不等式在某状态严格成立。
3. **保持 proper**：本实现对 `mu'` 显式做 `check_policy_proper`；在 robust SSP 假设下对 proper 策略做严格改进必得 proper 策略（否则其某状态值为 `+∞`，与 `J_{mu'} ≤ J_mu < ∞` 矛盾），代码亦以此为不变量（若意外不满足则优雅返回 `converged=false`）。
4. **终止性**：proper 策略只有有限多个；每轮要么严格降低某状态的值、要么停止，故不会无限循环，算法在有限轮内收敛。
5. **停止即最优**：若某轮 improvement 后策略不变，则对每个节点 `mu(x) ∈ argmin_u H(x,u,J_mu)`，即 `J_mu = T J_mu`，`J_mu` 满足 Bellman 最优性方程。
6. 由 §3 的不动点唯一性（SSP 假设下），`J_mu = J*`，故 `mu` 是 robust optimal policy。

## 5. Dijkstra-like 标号算法正确性

### 命题

在非负转移代价下，Dijkstra-like label-setting 每次永久标号的节点值即为其 robust optimal value，最终（若图 globally proper）得到与 VI 相同的 `J*`。

### 证明思路

1. 初始只有 terminal 被永久标号，其值为 `0`。
2. 一个 action `u` 仅当其**全部** successors 都已永久标号时才成为候选，其候选值为 `H(x,u)=max_{y∈Y(x,u)}[g(x,u,y)+J(y)]`（用已确定的 `J`）。
3. **关键性质（可永久标号）**：在所有候选中取最小值 `m` 永久标号是安全的。因为代价非负，任何尚未确定的后继 `y'` 的最终值 `J(y')` 都 `≥ m`（它将在之后以更大或相等的候选值被标号）；因此任何使用未确定后继的 action 其值 `≥ 0 + m = m`，不可能小于当前最小候选。故当前最小候选已是该节点的最终（最优）值。
4. **正确性**：归纳地，每个被永久标号的节点值都等于其 robust optimal value，与 Bellman 最优性方程一致。
5. **完整性 / 失败语义**：若某轮没有任何节点的某个 action 的全部后继都已标号，则这些节点在对抗语义下无法保证到达 terminal（无 proper 子结构），算法返回 `success=false`——与 VI 的 `all_values_finite=false` 语义一致。
6. 与 §3 的唯一不动点一致，故 globally proper 图上 Dijkstra-like 与 VI/PI 得到同一组 `J*`（实现中已用 `tie-break` 保证确定性）。

## 6. Exhaustive Search 作为 ground truth 的正确性

### 命题

在小规模 globally-proper 图上，枚举所有 proper stationary policy 并对每个状态取其值的逐点最小，得到的就是 robust optimal value function `J*`；它是验证 VI/PI/Dijkstra 的 ground-truth oracle。

### 证明思路

1. robust SSP 的最优值函数由**某个单一最优 stationary policy** 同时在所有状态取得（最优策略存在性，见 §3/参考文献）。
2. 因此 `J*(x) = min_{μ proper} J_μ(x)` 对每个状态成立——这正是 `exhaustive_search` 计算的 `optimal_value_by_state`（对所有 proper policy 的逐点取最小）。
3. 每个 proper policy 的值由 §2 的 DAG 评估精确得到，故枚举所得逐点最小即 `J*`。
4. **适用边界**：该等式以"存在 proper policy"为前提。若图含无法到达 terminal 的 trap 节点（非 globally proper），exhaustive 在该状态返回 `INF`，此时应以 VI/Dijkstra 的 per-start 值为准；代码中 exhaustive 仅用于小规模 globally-proper 验证，并设有规模守卫（超过阈值返回 `success=false`，避免指数枚举卡死）。

## 7. Toy Example 结论为什么成立

### 命题

在 toy graph 上，节点 0 的 robust optimal action 是 `safe`，不是 `fast_risky`。

### 证明思路

先求其余节点值：

```text
J(5)=0,
J(1)=1,
J(4)=100,
J(2)=101,
J(3)=4
```

然后比较节点 0 的两个动作：

```text
fast_risky = max(1 + J(1), 1 + J(2)) = 102
safe       = 3 + J(3) = 7
```

因此节点 0 取 `safe` 更优，得到 `J(0)=7`。

这个例子说明：即使某个 action 的某条路径非常短，只要它允许对抗者把系统导向代价极大的分支，就不会是 robust optimal action。

## 8. 实验设计合理性说明

### 实验 3

- 使用 layered DAG 可以保证存在 proper policy，避免把实验时间浪费在无效图实例上。
- 所有算法在同一批图上运行，因此 runtime 与 iterations 的对比具有可比性。

### 实验 4

- 控制 `s` 可以直接调节 uncertainty 强度。
- 比较指标使用 adversarial rollout 的 worst-case cost，而不是 nominal shortest-path cost，更符合 RSP 定义。
- 当 `s=1` 时所有方法应一致，这一现象可作为 sanity check。