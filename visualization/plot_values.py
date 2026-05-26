from __future__ import annotations

import argparse
import math

from common import PALETTE, SVGCanvas, draw_grouped_bar_chart, load_values, save_svg


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot value comparisons from values.csv.")
    parser.add_argument("--values-csv", required=True, help="Path to values.csv.")
    parser.add_argument("--graph-id", required=True, help="Graph id to plot.")
    parser.add_argument(
        "--algorithms",
        default="vi,pi,dijkstra,exhaustive",
        help="Comma-separated algorithms to compare.",
    )
    parser.add_argument("--output", required=True, help="Path to output figure.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    algorithms = [item.strip() for item in args.algorithms.split(",") if item.strip()]
    series = {algorithm: load_values(args.values_csv, args.graph_id, algorithm) for algorithm in algorithms}
    nodes = sorted({node for values in series.values() for node in values})
    chart_series = [
        {
            "name": algorithm,
            "color": PALETTE[index % len(PALETTE)],
            "values": [series[algorithm].get(node, math.inf) for node in nodes],
        }
        for index, algorithm in enumerate(algorithms)
    ]
    canvas = SVGCanvas(1000, 560)
    draw_grouped_bar_chart(
        canvas,
        (36.0, 32.0, 928.0, 492.0),
        title=f"Value Comparison on {args.graph_id}",
        x_labels=[f"node {node}" for node in nodes],
        series=chart_series,
        y_label="Worst-case value",
    )
    save_svg(args.output, canvas.to_svg())


if __name__ == "__main__":
    main()