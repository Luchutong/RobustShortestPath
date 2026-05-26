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

### 证明思路

1. Bellman 算子 `T` 定义为逐点取 action 的最小 worst-case value。
2. 每次迭代 `J_{k+1} = T J_k` 都不会改变 terminal 条件。
3. 在实验使用的 layered DAG 或存在 proper policy 的图上，`T` 的反复应用逼近其最小不动点。
4. 极限点必须满足 `J = T J`，即 Bellman 最优性方程。
5. 最后从 `J*` 抽取 greedy policy，即得 robust optimal policy。

注：最终正式报告中可根据课程要求，把这部分写成更严格的单调性、压缩性或 Bertsekas 文献中的命题形式。

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