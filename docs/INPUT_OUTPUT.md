# 输入输出格式

## TXT 输入

默认图输入格式：

```text
n terminal
for node 0:
    number_of_actions
    action_id number_of_successors successor_id cost successor_id cost ...
for node 1:
    ...
```

toy example：

```text
6 5
2
0 2 1 1.0 2 1.0
1 1 3 3.0
1
0 1 5 1.0
1
0 1 4 1.0
1
0 1 5 4.0
1
0 1 5 100.0
0
```

解释：

- 第一行 `6 5` 表示共有 6 个节点，终点是节点 5。
- 每个节点先写 action 数量。
- 每个 action 一行：`action_id successor_count to cost to cost ...`.
- terminal 节点 action 数为 `0`.

## JSON 图格式

JSON 第一阶段主要用于可视化，不作为 C++ 主输入：

```json
{
  "n": 6,
  "terminal": 5,
  "nodes": [
    {
      "id": 0,
      "actions": [
        {
          "action_id": 0,
          "name": "fast_risky",
          "successors": [
            {"to": 1, "cost": 1.0},
            {"to": 2, "cost": 1.0}
          ]
        }
      ]
    }
  ]
}
```

## CSV 输出

`values.csv`

```csv
graph_id,algorithm,node,value
toy_graph,vi,0,7.000000
```

`policies.csv`

```csv
graph_id,algorithm,node,action_index,action_id
toy_graph,vi,0,1,1
```

`residual_history.csv`

```csv
graph_id,algorithm,iteration,residual
toy_graph,vi,1,inf
```

`runtime.csv`

```csv
graph_id,n,total_actions,total_transitions,algorithm,runtime_ms,iterations,converged,success,avg_value
toy_graph,6,6,7,vi,0.052000,4,1,1,42.600000
```

`runtime_experiment.csv`

```csv
graph_id,n,total_actions,total_transitions,algorithm,runtime_ms,iterations,converged,success,avg_value
medium_n20_seed42,20,57,114,vi,0.120000,8,1,1,50.123456
```

`runtime_summary.csv`

```csv
n,algorithm,cases,success_count,success_rate,avg_runtime_ms,avg_iterations,avg_value
20,vi,20,20,1.000000,0.120000,8.000000,50.123456
```

`robustness.csv`

```csv
graph_id,s,policy_type,start_node,policy_valid,status,worst_cost,terminated,steps
toy_graph,2,baseline_nominal,0,1,ok,102.000000,1,3
toy_graph,2,vi,0,1,ok,7.000000,1,2
```

`robustness_summary.csv`

```csv
s,policy_type,cases,valid_count,valid_rate,terminated_count,terminated_rate,avg_worst_cost,avg_steps
2,baseline_nominal,20,20,1.000000,20,1.000000,57.428797,5.000000
2,vi,20,20,1.000000,20,1.000000,40.032566,4.450000
```

## 命令行接口

```bash
./build/rsp_main --input data/toy_graph.txt --algorithm vi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm pi --output results
./build/rsp_main --input data/toy_graph.txt --algorithm dijkstra --output results
./build/rsp_main --input data/toy_graph.txt --algorithm exhaustive --output results
./build/rsp_main --input data/toy_graph.txt --algorithm baseline_nominal --output results
./build/rsp_main --input data/toy_graph.txt --algorithm baseline_bestcase --output results
./build/rsp_main --input data/toy_graph.txt --algorithm baseline_worst_immediate --output results
python3 experiments/generate_medium_graphs.py --output data/random_graphs
./build/run_runtime --input-dir data/random_graphs --output results
./build/run_robustness --input data/toy_graph.txt --output results --start 0 --max-steps 20
./build/run_robustness --input-dir experiment_data/official_20260521_210335/exp4_robustness/graphs --output results --start 0 --max-steps 1000
```

`run_runtime` 可选参数：

```bash
--input-dir data/random_graphs
--output results
--max-iter 100000
--epsilon 1e-9
```

其中 `--max-iter` 必须非负，`--epsilon` 必须为正。

`run_robustness` 可选参数：

```bash
--input-dir experiment_data/official_20260521_210335/exp4_robustness/graphs
--s 2
```

如果不传 `--s`，程序默认用图中最大 successor set size 作为 `s`。目录模式下不允许再传 `--s` 覆盖整批图的标记。`--max-steps` 必须非负。

算法名统一使用：

```text
vi
pi
dijkstra
exhaustive
baseline_nominal
baseline_bestcase
baseline_worst_immediate
```
