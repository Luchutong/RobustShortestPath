# 理论笔记

## 1. 问题定义

Robust Shortest Path, RSP, 与普通最短路的区别在于：在节点 `x` 选择动作 `u` 后，系统不会进入唯一确定的后继节点，而是由对抗者从不确定 successor 集合 `Y(x, u)` 中选择最坏的后继 `y`。

因此，RSP 的目标不是求一条 path，而是求一个 stationary policy `mu`，使得在所有可能 successor 选择下，达到 terminal 的 worst-case total cost 最小。

## 2. Bellman 方程

对任意 value function `J`，定义 action value：

```text
H(x, u, J) = max_{y in Y(x, u)} [g(x, u, y) + J(y)]
```

对应的 Bellman 算子为：

```text
(T J)(x) = min_{u in U(x)} H(x, u, J)
```

终点满足：

```text
J(t) = 0
```

在 proper policy 存在且所有 improper policy 代价可区分的前提下，最优值函数 `J*` 满足 Bellman 不动点方程：

```text
J*(x) = min_{u in U(x)} max_{y in Y(x, u)} [g(x, u, y) + J*(y)]
```

## 3. Proper Policy 的作用

proper policy 的直观含义是：无论对抗者如何选择 successor，系统都能在有限步内到达 terminal，而不会陷入环或死锁。

在工程实现里，properness 至少要覆盖两类检查：

- 策略诱导的非终点子图无环。
- 所有非终点节点都能沿策略到达 terminal。

没有 properness 约束时，value function 可能为无穷，policy iteration 的 policy evaluation 也无法在 DAG 上直接反向计算。

## 4. 三类核心算法的关系

### 4.1 Value Iteration

Value Iteration 重复应用 Bellman 算子：

```text
J_{k+1} = T J_k
```

其优点是实现直接、鲁棒，缺点是可能需要多轮迭代才能达到指定收敛阈值。

### 4.2 Policy Iteration

Policy Iteration 在两步之间交替：

1. policy evaluation：对当前 proper policy 精确求值；
2. policy improvement：对每个节点选择在当前 value 下更优的 action。

如果 improvement 后策略不再变化，则得到一个满足 Bellman 最优性的 policy。

### 4.3 Dijkstra-like

Dijkstra-like 算法只适用于非负边权，并利用“所有 successors 已知后才可确定当前节点值”的 label-setting 思想。对 layered DAG 或结构较规整的非负图，它可以一次 finalized 一个节点；但当前仓库实现是 naive 扫描版，因此理论上和工程上都不必然优于 VI。

## 5. Deterministic Baseline 与 Robust Policy 的区别

deterministic baselines 先把不确定 successor 集合确定化，再按普通 shortest path 的思想规划。这样做忽略了对抗者可在执行时重新选择最坏 successor 的事实。

robust policy 则在 Bellman 更新中直接把最坏 successor 纳入优化目标，因此在存在 uncertainty 时通常更保守，但能显著降低 adversarial rollout 下的 worst-case cost。

## 6. 与实验现象的对应

toy example 与实验 4 体现了同一个理论事实：

- 如果 `s = 1`，则 RSP 退化为 deterministic shortest path，baseline 与 robust policy 一致。
- 如果 `s > 1`，对抗者自由度上升，deterministic planner 的名义最优 action 可能不再是 robust optimal。

因此，RSP 的研究重点不是“单条最短路”，而是“在最坏转移选择下仍然可接受的策略”。