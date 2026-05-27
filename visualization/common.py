from __future__ import annotations

import csv
import html
import json
import math
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


PALETTE = [
    "#1f4e5f",
    "#c75b39",
    "#768948",
    "#99627a",
    "#4b5563",
]
NODE_FILL = "#f4efe6"
TERMINAL_FILL = "#d8efe0"
NODE_BORDER = "#274c5e"
MUTED_EDGE = "#c7d0d9"
PAGE_BG = "#fffaf2"
TEXT_DARK = "#22313f"


def graph_id_from_path(path: str) -> str:
    return Path(path).stem


def ensure_parent_dir(path: str) -> None:
    Path(path).parent.mkdir(parents=True, exist_ok=True)


def load_graph_json(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def read_csv_rows(path: str) -> List[dict]:
    with open(path, "r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def load_values(path: str, graph_id: str, algorithm: str) -> Dict[int, float]:
    rows = read_csv_rows(path)
    values: Dict[int, float] = {}
    for row in rows:
        if row["graph_id"] != graph_id or row["algorithm"] != algorithm:
            continue
        value = row["value"].strip().lower()
        values[int(row["node"])] = math.inf if value == "inf" else float(value)
    if not values:
        raise ValueError(f"no values for graph_id={graph_id}, algorithm={algorithm}")
    return values


def load_policy(path: str, graph_id: str, algorithm: str) -> Dict[int, int]:
    rows = read_csv_rows(path)
    policy: Dict[int, int] = {}
    for row in rows:
        if row["graph_id"] != graph_id or row["algorithm"] != algorithm:
            continue
        policy[int(row["node"])] = int(row["action_index"])
    if not policy:
        raise ValueError(f"no policy for graph_id={graph_id}, algorithm={algorithm}")
    return policy


def load_runtime_summary(path: str) -> List[dict]:
    rows = read_csv_rows(path)
    for row in rows:
        row["n"] = int(row["n"])
        row["cases"] = int(row["cases"])
        row["success_count"] = int(row["success_count"])
        row["success_rate"] = float(row["success_rate"])
        row["avg_runtime_ms"] = parse_float(row["avg_runtime_ms"])
        row["avg_iterations"] = parse_float(row["avg_iterations"])
        row["avg_value"] = parse_float(row["avg_value"])
    return rows


def load_robustness(path: str, graph_id: str | None = None) -> List[dict]:
    rows = read_csv_rows(path)
    filtered = []
    for row in rows:
        if graph_id is not None and row["graph_id"] != graph_id:
            continue
        row["s"] = int(row["s"])
        row["start_node"] = int(row["start_node"])
        row["policy_valid"] = int(row.get("policy_valid", "1"))
        row["invalid_reason"] = row.get("invalid_reason", "")
        row["worst_cost"] = parse_float(row["worst_cost"])
        row["terminated"] = int(row["terminated"])
        row["steps"] = int(row["steps"])
        filtered.append(row)
    return filtered


def load_robustness_summary(path: str) -> List[dict]:
    rows = read_csv_rows(path)
    for row in rows:
        row["s"] = int(row["s"])
        row["cases"] = int(row["cases"])
        row["valid_count"] = int(row.get("valid_count", row["cases"]))
        row["valid_rate"] = float(row.get("valid_rate", "1.0"))
        row["terminated_count"] = int(row["terminated_count"])
        row["terminated_rate"] = float(row["terminated_rate"])
        row["avg_worst_cost"] = parse_float(row["avg_worst_cost"])
        row["avg_steps"] = parse_float(row["avg_steps"])
    return rows


def parse_float(value: str) -> float:
    return math.inf if value.strip().lower() == "inf" else float(value)


def compute_node_positions(graph: dict) -> Dict[int, Tuple[float, float]]:
    node_ids = [node["id"] for node in graph["nodes"]]
    outgoing = {node_id: set() for node_id in node_ids}
    indegree = {node_id: 0 for node_id in node_ids}
    for node in graph["nodes"]:
        for action in node["actions"]:
            for successor in action["successors"]:
                to_node = successor["to"]
                if to_node not in outgoing[node["id"]]:
                    outgoing[node["id"]].add(to_node)
                    indegree[to_node] += 1

    roots = [node_id for node_id in node_ids if indegree[node_id] == 0]
    if not roots:
        roots = [min(node_ids)]

    depths = {node_id: 0 for node_id in roots}
    queue = list(roots)
    while queue:
        current = queue.pop(0)
        for nxt in sorted(outgoing[current]):
            candidate = depths[current] + 1
            if nxt not in depths or candidate > depths[nxt]:
                depths[nxt] = candidate
                queue.append(nxt)

    for node_id in node_ids:
        depths.setdefault(node_id, 0)

    levels: Dict[int, List[int]] = {}
    for node_id, depth in depths.items():
        levels.setdefault(depth, []).append(node_id)

    positions: Dict[int, Tuple[float, float]] = {}
    max_width = max(len(nodes) for nodes in levels.values()) if levels else 1
    for depth, nodes in sorted(levels.items()):
        ordered = sorted(nodes)
        span = max_width - 1 if max_width > 1 else 1
        for index, node_id in enumerate(ordered):
            if len(ordered) == 1:
                y_value = 0.0
            else:
                y_value = span / 2.0 - index * (span / (len(ordered) - 1))
            positions[node_id] = (float(depth), float(y_value))
    return positions


def action_label(action: dict, action_index: int) -> str:
    name = action.get("name", "").strip()
    if name:
        return name
    return f"a{action.get('action_id', action_index)}"


def edge_midpoint(start: Tuple[float, float], end: Tuple[float, float], offset: float) -> Tuple[float, float]:
    mid_x = (start[0] + end[0]) / 2.0
    mid_y = (start[1] + end[1]) / 2.0 + offset
    return mid_x, mid_y


def path_pairs(path: Iterable[int]) -> set[Tuple[int, int]]:
    nodes = list(path)
    return {(nodes[index], nodes[index + 1]) for index in range(len(nodes) - 1)}


class SVGCanvas:
    def __init__(self, width: int, height: int, background: str = PAGE_BG) -> None:
        self.width = width
        self.height = height
        self.background = background
        self.elements: List[str] = [
            f'<rect x="0" y="0" width="{width}" height="{height}" fill="{background}" />'
        ]

    def add(self, element: str) -> None:
        self.elements.append(element)

    def line(
        self,
        x1: float,
        y1: float,
        x2: float,
        y2: float,
        *,
        stroke: str,
        stroke_width: float = 1.5,
        opacity: float = 1.0,
        dasharray: str | None = None,
    ) -> None:
        dash = f' stroke-dasharray="{dasharray}"' if dasharray else ""
        self.add(
            f'<line x1="{x1:.2f}" y1="{y1:.2f}" x2="{x2:.2f}" y2="{y2:.2f}" '
            f'stroke="{stroke}" stroke-width="{stroke_width:.2f}" opacity="{opacity:.2f}"{dash} />'
        )

    def arrow(
        self,
        x1: float,
        y1: float,
        x2: float,
        y2: float,
        *,
        stroke: str,
        stroke_width: float = 1.5,
        opacity: float = 1.0,
    ) -> None:
        dx = x2 - x1
        dy = y2 - y1
        length = math.hypot(dx, dy)
        if length < 1e-6:
            return
        ux = dx / length
        uy = dy / length
        margin = 22.0
        start_x = x1 + ux * margin
        start_y = y1 + uy * margin
        end_x = x2 - ux * margin
        end_y = y2 - uy * margin
        self.line(start_x, start_y, end_x, end_y, stroke=stroke, stroke_width=stroke_width, opacity=opacity)
        arrow_len = 12.0
        arrow_half = 6.0
        left_x = end_x - ux * arrow_len - uy * arrow_half
        left_y = end_y - uy * arrow_len + ux * arrow_half
        right_x = end_x - ux * arrow_len + uy * arrow_half
        right_y = end_y - uy * arrow_len - ux * arrow_half
        self.add(
            '<polygon points="'
            f'{end_x:.2f},{end_y:.2f} {left_x:.2f},{left_y:.2f} {right_x:.2f},{right_y:.2f}'
            f'" fill="{stroke}" opacity="{opacity:.2f}" />'
        )

    def rect(
        self,
        x: float,
        y: float,
        width: float,
        height: float,
        *,
        fill: str,
        stroke: str = "none",
        stroke_width: float = 0.0,
        rx: float = 0.0,
        opacity: float = 1.0,
    ) -> None:
        self.add(
            f'<rect x="{x:.2f}" y="{y:.2f}" width="{width:.2f}" height="{height:.2f}" '
            f'fill="{fill}" stroke="{stroke}" stroke-width="{stroke_width:.2f}" rx="{rx:.2f}" opacity="{opacity:.2f}" />'
        )

    def circle(
        self,
        cx: float,
        cy: float,
        r: float,
        *,
        fill: str,
        stroke: str,
        stroke_width: float,
    ) -> None:
        self.add(
            f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="{r:.2f}" fill="{fill}" '
            f'stroke="{stroke}" stroke-width="{stroke_width:.2f}" />'
        )

    def polyline(
        self,
        points: Sequence[Tuple[float, float]],
        *,
        stroke: str,
        stroke_width: float,
        opacity: float = 1.0,
    ) -> None:
        if len(points) < 2:
            return
        point_str = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
        self.add(
            f'<polyline points="{point_str}" fill="none" stroke="{stroke}" '
            f'stroke-width="{stroke_width:.2f}" opacity="{opacity:.2f}" />'
        )

    def text(
        self,
        x: float,
        y: float,
        content: str,
        *,
        size: int = 12,
        fill: str = TEXT_DARK,
        anchor: str = "middle",
        weight: str = "normal",
    ) -> None:
        escaped = html.escape(content)
        self.add(
            f'<text x="{x:.2f}" y="{y:.2f}" text-anchor="{anchor}" '
            f'font-size="{size}" font-weight="{weight}" fill="{fill}" '
            'font-family="DejaVu Sans, Arial, sans-serif">'
            f'{escaped}</text>'
        )

    def to_svg(self) -> str:
        body = "\n  ".join(self.elements)
        return (
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.width}" height="{self.height}" '
            f'viewBox="0 0 {self.width} {self.height}">\n  {body}\n</svg>\n'
        )


def scale_positions(
    raw_positions: Dict[int, Tuple[float, float]],
    width: int,
    height: int,
    *,
    margin_left: float = 90.0,
    margin_right: float = 90.0,
    margin_top: float = 80.0,
    margin_bottom: float = 70.0,
) -> Dict[int, Tuple[float, float]]:
    x_values = [point[0] for point in raw_positions.values()]
    y_values = [point[1] for point in raw_positions.values()]
    min_x, max_x = min(x_values), max(x_values)
    min_y, max_y = min(y_values), max(y_values)
    usable_w = max(width - margin_left - margin_right, 1.0)
    usable_h = max(height - margin_top - margin_bottom, 1.0)

    def scale_x(value: float) -> float:
        if math.isclose(min_x, max_x):
            return margin_left + usable_w / 2.0
        return margin_left + (value - min_x) / (max_x - min_x) * usable_w

    def scale_y(value: float) -> float:
        if math.isclose(min_y, max_y):
            return margin_top + usable_h / 2.0
        return margin_top + (max_y - value) / (max_y - min_y) * usable_h

    return {node_id: (scale_x(x_value), scale_y(y_value)) for node_id, (x_value, y_value) in raw_positions.items()}


def render_graph_svg(
    graph: dict,
    *,
    values: Dict[int, float] | None = None,
    selected_policy: Dict[int, int] | None = None,
    path: List[int] | None = None,
    path_color: str = "#c2410c",
    title: str = "",
    width: int = 1000,
    height: int = 560,
) -> str:
    canvas = SVGCanvas(width, height)
    if title:
        canvas.text(width / 2.0, 36.0, title, size=24, weight="bold")

    positions = scale_positions(compute_node_positions(graph), width, height)
    highlighted_edges = path_pairs(path or [])

    for node in graph["nodes"]:
        start = positions[node["id"]]
        actions = node["actions"]
        for action_index, action in enumerate(actions):
            base_color = PALETTE[action_index % len(PALETTE)]
            chosen = selected_policy is None or selected_policy.get(node["id"], -1) == action_index
            for successor_index, successor in enumerate(action["successors"]):
                end = positions[successor["to"]]
                edge_pair = (node["id"], successor["to"])
                edge_color = path_color if edge_pair in highlighted_edges else (base_color if chosen else MUTED_EDGE)
                alpha = 0.98 if edge_pair in highlighted_edges else (0.9 if chosen else 0.52)
                stroke_width = 4.0 if edge_pair in highlighted_edges else (2.7 if chosen else 1.6)
                canvas.arrow(start[0], start[1], end[0], end[1], stroke=edge_color, stroke_width=stroke_width, opacity=alpha)
                label_offset = 16.0 * (action_index - max(len(actions) - 1, 0) / 2.0) + 10.0 * successor_index
                label_x = (start[0] + end[0]) / 2.0
                label_y = (start[1] + end[1]) / 2.0 + label_offset
                label = f"{action_label(action, action_index)} | {successor['cost']:.1f}"
                canvas.rect(label_x - 42.0, label_y - 14.0, 84.0, 20.0, fill="#ffffff", rx=5.0, opacity=0.82)
                canvas.text(label_x, label_y, label, size=10, fill=TEXT_DARK if chosen else "#8b98a5")

    radius = 18.0
    for node in graph["nodes"]:
        node_id = node["id"]
        x_value, y_value = positions[node_id]
        is_terminal = node_id == graph["terminal"]
        canvas.circle(
            x_value,
            y_value,
            radius,
            fill=TERMINAL_FILL if is_terminal else NODE_FILL,
            stroke=NODE_BORDER,
            stroke_width=2.4,
        )
        canvas.text(x_value, y_value + 4.0, f"T{node_id}" if is_terminal else str(node_id), size=14, weight="bold")
        if values is not None and node_id in values:
            value = values[node_id]
            value_text = "J=inf" if math.isinf(value) else f"J={value:.1f}"
            canvas.text(x_value, y_value + 38.0, value_text, size=11, fill="#334155")

    return canvas.to_svg()


def save_svg(output: str, svg_text: str) -> None:
    ensure_parent_dir(output)
    with open(output, "w", encoding="utf-8") as handle:
        handle.write(svg_text)


def _frame_mapping(frame: Tuple[float, float, float, float], max_value: float):
    x, y, width, height = frame
    left = x + 52.0
    right = x + width - 24.0
    top = y + 26.0
    bottom = y + height - 44.0
    usable_h = max(bottom - top, 1.0)

    def map_y(value: float) -> float:
        safe_max = max(max_value, 1.0)
        return bottom - (value / safe_max) * usable_h

    return left, right, top, bottom, map_y


def draw_panel(canvas: SVGCanvas, frame: Tuple[float, float, float, float], title: str) -> None:
    x, y, width, height = frame
    canvas.rect(x, y, width, height, fill="#fffdf9", stroke="#d3d8dd", stroke_width=1.2, rx=12.0)
    canvas.text(x + width / 2.0, y + 22.0, title, size=16, weight="bold")


def draw_grouped_bar_chart(
    canvas: SVGCanvas,
    frame: Tuple[float, float, float, float],
    *,
    title: str,
    x_labels: Sequence[str],
    series: Sequence[dict],
    y_label: str,
) -> None:
    draw_panel(canvas, frame, title)
    finite_values = [
        value
        for item in series
        for value in item["values"]
        if not math.isinf(value)
    ]
    max_value = max(finite_values) if finite_values else 1.0
    left, right, top, bottom, map_y = _frame_mapping(frame, max_value * 1.15)
    canvas.line(left, bottom, right, bottom, stroke="#94a3b8", stroke_width=1.4)
    canvas.line(left, top, left, bottom, stroke="#94a3b8", stroke_width=1.4)

    ticks = 5
    for tick in range(ticks + 1):
        value = max_value * tick / ticks
        y_pos = map_y(value)
        canvas.line(left, y_pos, right, y_pos, stroke="#e2e8f0", stroke_width=1.0, opacity=0.9, dasharray="5 5")
        canvas.text(left - 10.0, y_pos + 4.0, f"{value:.1f}", size=10, anchor="end", fill="#475569")
    canvas.text(frame[0] + 16.0, (top + bottom) / 2.0, y_label, size=11, anchor="middle", fill="#475569")

    group_width = (right - left) / max(len(x_labels), 1)
    bar_width = group_width * 0.72 / max(len(series), 1)
    for group_index, label in enumerate(x_labels):
        group_start = left + group_index * group_width + group_width * 0.14
        for series_index, item in enumerate(series):
            value = item["values"][group_index]
            height = 0.0 if math.isinf(value) else bottom - map_y(value)
            x_pos = group_start + series_index * bar_width
            y_pos = bottom - height
            canvas.rect(x_pos, y_pos, bar_width - 3.0, height, fill=item["color"], rx=5.0, opacity=0.9)
            label_text = "inf" if math.isinf(value) else f"{value:.1f}"
            canvas.text(x_pos + (bar_width - 3.0) / 2.0, y_pos - 6.0, label_text, size=9)
        canvas.text(group_start + group_width * 0.35, bottom + 22.0, label, size=10)

    legend_x = right - 110.0
    legend_y = top + 8.0
    for index, item in enumerate(series):
        canvas.rect(legend_x, legend_y + index * 20.0, 12.0, 12.0, fill=item["color"], rx=2.0)
        canvas.text(legend_x + 18.0, legend_y + 10.0 + index * 20.0, item["name"], size=10, anchor="start")


def draw_line_chart(
    canvas: SVGCanvas,
    frame: Tuple[float, float, float, float],
    *,
    title: str,
    x_values: Sequence[float],
    series: Sequence[dict],
    y_label: str,
) -> None:
    draw_panel(canvas, frame, title)
    finite_values = [
        value
        for item in series
        for value in item["values"]
        if not math.isnan(value) and not math.isinf(value)
    ]
    max_value = max(finite_values) if finite_values else 1.0
    left, right, top, bottom, map_y = _frame_mapping(frame, max_value * 1.1)
    canvas.line(left, bottom, right, bottom, stroke="#94a3b8", stroke_width=1.4)
    canvas.line(left, top, left, bottom, stroke="#94a3b8", stroke_width=1.4)
    ticks = 5
    for tick in range(ticks + 1):
        value = max_value * tick / ticks
        y_pos = map_y(value)
        canvas.line(left, y_pos, right, y_pos, stroke="#e2e8f0", stroke_width=1.0, opacity=0.9, dasharray="5 5")
        canvas.text(left - 10.0, y_pos + 4.0, f"{value:.2f}" if max_value <= 5 else f"{value:.1f}", size=10, anchor="end", fill="#475569")
    x_min = min(x_values) if x_values else 0.0
    x_max = max(x_values) if x_values else 1.0
    usable_w = max(right - left, 1.0)

    def map_x(value: float) -> float:
        if math.isclose(x_min, x_max):
            return left + usable_w / 2.0
        return left + (value - x_min) / (x_max - x_min) * usable_w

    for x_value in x_values:
        x_pos = map_x(x_value)
        canvas.line(x_pos, bottom, x_pos, bottom + 6.0, stroke="#94a3b8", stroke_width=1.0)
        canvas.text(x_pos, bottom + 22.0, str(int(x_value)), size=10)

    for item in series:
        points = []
        for x_value, y_value in zip(x_values, item["values"]):
            if math.isnan(y_value) or math.isinf(y_value):
                continue
            points.append((map_x(x_value), map_y(y_value)))
        canvas.polyline(points, stroke=item["color"], stroke_width=3.0)
        for x_pos, y_pos in points:
            canvas.circle(x_pos, y_pos, 4.0, fill=item["color"], stroke=item["color"], stroke_width=1.0)

    legend_x = right - 110.0
    legend_y = top + 8.0
    for index, item in enumerate(series):
        y_pos = legend_y + index * 20.0
        canvas.line(legend_x, y_pos, legend_x + 14.0, y_pos, stroke=item["color"], stroke_width=3.0)
        canvas.text(legend_x + 20.0, y_pos + 4.0, item["name"], size=10, anchor="start")
    canvas.text(frame[0] + 18.0, (top + bottom) / 2.0, y_label, size=11, anchor="middle", fill="#475569")
