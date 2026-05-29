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

### 证明思路

1. 当前策略 `mu` 的 evaluation 已给出其精确值 `J_mu`。
2. improvement 步骤对每个节点选择 `argmin_u H(x, u, J_mu)`。
3. 若 improvement 后策略不变，则说明对每个节点都有：

```text
mu(x) in argmin_u H(x, u, J_mu)
```

4. 因而 `J_mu` 同时满足策略方程与 Bellman 最优性方程。
5. 所以 `mu` 是最优策略，`J_mu = J*`。

## 5. Toy Example 结论为什么成立

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

## 6. 实验设计合理性说明

### 实验 3

- 使用 layered DAG 可以保证存在 proper policy，避免把实验时间浪费在无效图实例上。
- 所有算法在同一批图上运行，因此 runtime 与 iterations 的对比具有可比性。

### 实验 4

- 控制 `s` 可以直接调节 uncertainty 强度。
- 比较指标使用 adversarial rollout 的 worst-case cost，而不是 nominal shortest-path cost，更符合 RSP 定义。
- 当 `s=1` 时所有方法应一致，这一现象可作为 sanity check。