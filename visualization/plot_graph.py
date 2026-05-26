from __future__ import annotations

import argparse

from common import graph_id_from_path, load_graph_json, render_graph_svg, save_svg


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot a graph JSON as an action graph.")
    parser.add_argument("--graph-json", required=True, help="Path to graph JSON input.")
    parser.add_argument("--output", required=True, help="Path to output figure.")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    graph = load_graph_json(args.graph_json)
    svg_text = render_graph_svg(graph, title=f"Graph Structure: {graph_id_from_path(args.graph_json)}")
    save_svg(args.output, svg_text)


if __name__ == "__main__":
    main()