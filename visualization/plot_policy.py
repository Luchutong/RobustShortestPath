from __future__ import annotations

import argparse

from common import load_graph_json, load_policy, load_values, render_graph_svg, run_cli, save_svg


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot a selected policy on top of a graph JSON.")
    parser.add_argument("--graph-json", required=True, help="Path to graph JSON input.")
    parser.add_argument("--policies-csv", required=True, help="Path to policies.csv.")
    parser.add_argument("--values-csv", required=True, help="Path to values.csv.")
    parser.add_argument("--graph-id", required=True, help="Graph id to plot.")
    parser.add_argument("--algorithm", required=True, help="Algorithm name.")
    parser.add_argument("--output", required=True, help="Path to output figure.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    graph = load_graph_json(args.graph_json)
    policy = load_policy(args.policies_csv, args.graph_id, args.algorithm)
    values = load_values(args.values_csv, args.graph_id, args.algorithm)

    svg_text = render_graph_svg(
        graph,
        values=values,
        selected_policy=policy,
        title=f"Policy View: {args.algorithm} on {args.graph_id}",
    )
    save_svg(args.output, svg_text)


if __name__ == "__main__":
    run_cli(main)