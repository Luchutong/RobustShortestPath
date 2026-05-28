from __future__ import annotations

import argparse
import math

from common import PALETTE, SVGCanvas, draw_grouped_bar_chart, draw_line_chart, load_robustness, load_robustness_summary, load_runtime_summary, save_svg


ALGORITHM_ORDER = ["vi", "pi", "dijkstra"]
ROBUSTNESS_ORDER = ["baseline_nominal", "baseline_bestcase", "baseline_worst_immediate", "vi"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot runtime and robustness comparisons.")
    parser.add_argument("--runtime-summary-csv", required=True, help="Path to runtime_summary.csv.")
    parser.add_argument("--robustness-csv", help="Optional path to robustness.csv.")
    parser.add_argument("--robustness-summary-csv", help="Optional path to robustness_summary.csv.")
    parser.add_argument("--graph-id", help="Graph id for robustness plot.")
    parser.add_argument("--requested-s", type=int, help="Filter runtime rows by requested_s.")
    parser.add_argument("--actions", type=int, help="Filter runtime rows by actions.")
    parser.add_argument("--output", required=True, help="Path to output figure.")
    return parser.parse_args()


def metric_series(rows: list[dict], metric: str) -> tuple[list[int], list[dict]]:
    sizes = sorted({row["n"] for row in rows})
    series = []
    for index, algorithm in enumerate(ALGORITHM_ORDER):
        algorithm_rows = [row for row in rows if row["algorithm"] == algorithm]
        if not algorithm_rows:
            continue
        mapping = {row["n"]: row[metric] for row in algorithm_rows}
        series.append(
            {
                "name": algorithm,
                "color": PALETTE[index % len(PALETTE)],
                "values": [mapping.get(size, math.nan) for size in sizes],
            }
        )
    return sizes, series


def robustness_series(rows: list[dict], graph_id: str | None) -> tuple[list[str], list[dict]]:
    selected = rows if graph_id is None else [row for row in rows if row["graph_id"] == graph_id]
    ordered = []
    for algorithm in ROBUSTNESS_ORDER:
        for row in selected:
            if row["policy_type"] == algorithm:
                ordered.append(row)
                break
    labels = [row["policy_type"] for row in ordered]
    values = [row["worst_cost"] for row in ordered]
    return labels, [{"name": "worst_cost", "color": PALETTE[0], "values": values}]


def robustness_summary_series(rows: list[dict]) -> tuple[list[int], list[dict]]:
    s_values = sorted({row["s"] for row in rows})
    series = []
    for index, algorithm in enumerate(ROBUSTNESS_ORDER):
        algorithm_rows = [row for row in rows if row["policy_type"] == algorithm]
        if not algorithm_rows:
            continue
        mapping = {row["s"]: row["avg_worst_cost"] for row in algorithm_rows}
        series.append(
            {
                "name": algorithm,
                "color": PALETTE[index % len(PALETTE)],
                "values": [mapping.get(s_value, math.nan) for s_value in s_values],
            }
        )
    return s_values, series


def main() -> None:
    args = parse_args()
    runtime_rows = load_runtime_summary(args.runtime_summary_csv)

    if args.requested_s is not None:
        runtime_rows = [r for r in runtime_rows if int(r.get("requested_s", 0)) == args.requested_s]
    if args.actions is not None:
        runtime_rows = [r for r in runtime_rows if int(r.get("actions", 0)) == args.actions]

    canvas = SVGCanvas(1200, 860)
    canvas.text(600.0, 32.0, "Runtime and Correctness Comparison", size=24, weight="bold")

    sizes, runtime_series = metric_series(runtime_rows, "avg_runtime_ms")
    _, iteration_series = metric_series(runtime_rows, "avg_iterations")
    _, success_series = metric_series(runtime_rows, "success_rate")
    draw_line_chart(canvas, (30.0, 60.0, 555.0, 350.0), title="Average Runtime", x_values=sizes, series=runtime_series, y_label="runtime_ms")
    draw_line_chart(canvas, (615.0, 60.0, 555.0, 350.0), title="Average Iterations", x_values=sizes, series=iteration_series, y_label="iterations")
    draw_line_chart(canvas, (30.0, 440.0, 555.0, 350.0), title="Success Rate", x_values=sizes, series=success_series, y_label="success_rate")

    if args.robustness_summary_csv:
        summary_rows = load_robustness_summary(args.robustness_summary_csv)
        s_values, robustness_lines = robustness_summary_series(summary_rows)
        draw_line_chart(
            canvas,
            (615.0, 440.0, 555.0, 350.0),
            title="Average Worst-case Cost by s",
            x_values=s_values,
            series=robustness_lines,
            y_label="avg_worst_cost",
        )
    elif args.robustness_csv:
        robustness_rows = load_robustness(args.robustness_csv, args.graph_id)
        labels, series = robustness_series(robustness_rows, args.graph_id)
        if labels:
            colored_series = [
                {
                    "name": label,
                    "color": PALETTE[index % len(PALETTE)],
                    "values": [values],
                }
                for index, (label, values) in enumerate(zip(labels, series[0]["values"]))
            ]
            draw_grouped_bar_chart(
                canvas,
                (615.0, 440.0, 555.0, 350.0),
                title="Adversarial Worst-case Cost",
                x_labels=[args.graph_id or "graph"],
                series=colored_series,
                y_label="worst_cost",
            )
        else:
            _, avg_value_series = metric_series(runtime_rows, "avg_value")
            draw_line_chart(canvas, (615.0, 440.0, 555.0, 350.0), title="Average Value", x_values=sizes, series=avg_value_series, y_label="avg_value")
    else:
        _, avg_value_series = metric_series(runtime_rows, "avg_value")
        draw_line_chart(canvas, (615.0, 440.0, 555.0, 350.0), title="Average Value", x_values=sizes, series=avg_value_series, y_label="avg_value")

    save_svg(args.output, canvas.to_svg())


if __name__ == "__main__":
    main()