from __future__ import annotations

import argparse
import math

from common import load_graph_json, load_policy, load_robustness, load_values, render_graph_svg, save_svg


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot nominal path, adversarial path, and robust policy on the toy graph.")
    parser.add_argument("--graph-json", required=True, help="Path to toy graph JSON.")
    parser.add_argument("--policies-csv", required=True, help="Path to policies.csv.")
    parser.add_argument("--values-csv", required=True, help="Path to values.csv.")
    parser.add_argument("--robustness-csv", help="Optional path to robustness.csv for cost labels.")
    parser.add_argument("--graph-id", required=True, help="Graph id to visualize.")
    parser.add_argument("--start", type=int, default=0, help="Start node.")
    parser.add_argument("--max-steps", type=int, default=20, help="Maximum rollout steps.")
    parser.add_argument("--nominal-algorithm", default="baseline_nominal", help="Algorithm used for nominal baseline.")
    parser.add_argument("--robust-algorithm", default="vi", help="Algorithm used for robust policy.")
    parser.add_argument("--output", required=True, help="Path to output figure.")
    return parser.parse_args()


def nominal_path(graph: dict, policy: dict[int, int], start: int, max_steps: int) -> list[int]:
    path = [start]
    current = start
    terminal = graph["terminal"]
    for _ in range(max_steps):
        if current == terminal:
            break
        action_index = policy.get(current, -1)
        if action_index < 0:
            break
        action = graph["nodes"][current]["actions"][action_index]
        if not action["successors"]:
            break
        current = action["successors"][0]["to"]
        path.append(current)
        if current == terminal:
            break
    return path


def adversarial_path(graph: dict, policy: dict[int, int], values: dict[int, float], start: int, max_steps: int) -> list[int]:
    path = [start]
    current = start
    terminal = graph["terminal"]
    for _ in range(max_steps):
        if current == terminal:
            break
        action_index = policy.get(current, -1)
        if action_index < 0:
            break
        action = graph["nodes"][current]["actions"][action_index]
        best_to = None
        best_score = -math.inf
        for successor in action["successors"]:
            score = successor["cost"] + values.get(successor["to"], math.inf)
            if score > best_score or (math.isclose(score, best_score) and (best_to is None or successor["to"] < best_to)):
                best_score = score
                best_to = successor["to"]
        if best_to is None:
            break
        current = best_to
        path.append(current)
        if current == terminal:
            break
    return path


def cost_lookup(rows: list[dict], policy_type: str) -> str:
    for row in rows:
        if row["policy_type"] == policy_type:
            if math.isinf(row["worst_cost"]):
                return "inf"
            return f"{row['worst_cost']:.1f}"
    return "n/a"


def main() -> None:
    args = parse_args()
    graph = load_graph_json(args.graph_json)
    nominal_policy_map = load_policy(args.policies_csv, args.graph_id, args.nominal_algorithm)
    nominal_values_map = load_values(args.values_csv, args.graph_id, args.nominal_algorithm)
    robust_policy_map = load_policy(args.policies_csv, args.graph_id, args.robust_algorithm)
    robust_values_map = load_values(args.values_csv, args.graph_id, args.robust_algorithm)

    baseline_nominal_path = nominal_path(graph, nominal_policy_map, args.start, args.max_steps)
    baseline_adversarial_path = adversarial_path(graph, nominal_policy_map, nominal_values_map, args.start, args.max_steps)
    robust_adversarial_path = adversarial_path(graph, robust_policy_map, robust_values_map, args.start, args.max_steps)

    robustness_rows = load_robustness(args.robustness_csv, args.graph_id) if args.robustness_csv else []
    nominal_cost = cost_lookup(robustness_rows, args.nominal_algorithm)
    robust_cost = cost_lookup(robustness_rows, args.robust_algorithm)

    panels = [
        render_graph_svg(
            graph,
            selected_policy=nominal_policy_map,
            path=baseline_nominal_path,
            path_color="#2f855a",
            title="Nominal Path",
            width=1000,
            height=520,
        ),
        render_graph_svg(
            graph,
            selected_policy=nominal_policy_map,
            path=baseline_adversarial_path,
            path_color="#c2410c",
            title=f"Adversarial Path, cost={nominal_cost}",
            width=1000,
            height=520,
        ),
        render_graph_svg(
            graph,
            values=robust_values_map,
            selected_policy=robust_policy_map,
            path=robust_adversarial_path,
            path_color="#1d4ed8",
            title=f"Robust Policy, cost={robust_cost}",
            width=1000,
            height=520,
        ),
    ]
    stripped = []
    for panel in panels:
        content = panel.split(">", 1)[1].rsplit("</svg>", 1)[0]
        stripped.append(content)

    svg_text = (
        '<svg xmlns="http://www.w3.org/2000/svg" width="3000" height="520" viewBox="0 0 3000 520">'
        '<rect x="0" y="0" width="3000" height="520" fill="#fffaf2" />'
        f'<g transform="translate(0,0)">{stripped[0]}</g>'
        f'<g transform="translate(1000,0)">{stripped[1]}</g>'
        f'<g transform="translate(2000,0)">{stripped[2]}</g>'
        '</svg>\n'
    )
    save_svg(args.output, svg_text)


if __name__ == "__main__":
    main()