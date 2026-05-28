#!/usr/bin/env python3
"""Generate layered medium-size RSP graphs for runtime experiments."""

from __future__ import annotations

import argparse
import csv
import math
import random
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate layered random Robust Shortest Path graphs."
    )
    parser.add_argument("--output", default="data/random_graphs")
    parser.add_argument("--sizes", nargs="+", type=int, default=[20, 50, 100, 200])
    parser.add_argument("--cases", type=int, default=20)
    parser.add_argument("--actions", type=int, default=3)
    parser.add_argument("--successors", type=int, default=2)
    parser.add_argument("--successors-values", nargs="+", type=int, default=None)
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--min-cost", type=float, default=1.0)
    parser.add_argument("--max-cost", type=float, default=20.0)
    return parser.parse_args()


def build_layers(n: int) -> list[list[int]]:
    nonterminal_count = n - 1
    layer_count = max(2, min(10, nonterminal_count // 4))
    layers: list[list[int]] = [[] for _ in range(layer_count)]
    for node in range(nonterminal_count):
        layer = min(layer_count - 1, node * layer_count // nonterminal_count)
        layers[layer].append(node)
    return [layer for layer in layers if layer]


def later_nodes(layers: list[list[int]], layer_index: int, terminal: int) -> list[int]:
    candidates: list[int] = []
    for layer in layers[layer_index + 1 :]:
        candidates.extend(layer)
    candidates.append(terminal)
    return candidates


def choose_successors(
    rng: random.Random,
    candidates: list[int],
    count: int,
    forced: int | None = None,
) -> list[int]:
    ordered_candidates = list(dict.fromkeys(candidates))
    successors: list[int] = []
    if forced is not None and forced in ordered_candidates:
        successors.append(forced)
        ordered_candidates = [node for node in ordered_candidates if node != forced]

    remaining = max(0, count - len(successors))
    if remaining == 0:
        return successors
    if not ordered_candidates:
        return successors

    if remaining >= len(ordered_candidates):
        successors.extend(ordered_candidates)
    else:
        successors.extend(rng.sample(ordered_candidates, remaining))
    return successors


def random_cost(rng: random.Random, min_cost: float, max_cost: float) -> float:
    return round(rng.uniform(min_cost, max_cost), 6)


def write_graph(
    path: Path,
    n: int,
    rng: random.Random,
    actions_per_node: int,
    successors_per_action: int,
    min_cost: float,
    max_cost: float,
) -> dict[str, float]:
    terminal = n - 1
    layers = build_layers(n)
    node_layer = {
        node: layer_index
        for layer_index, layer in enumerate(layers)
        for node in layer
    }
    actual_successor_counts: list[int] = []

    with path.open("w", encoding="utf-8") as out:
        out.write(f"{n} {terminal}\n")
        for node in range(n):
            if node == terminal:
                out.write("0\n")
                continue

            out.write(f"{actions_per_node}\n")
            layer_index = node_layer[node]
            candidates = later_nodes(layers, layer_index, terminal)
            safe_next = candidates[0]

            for action in range(actions_per_node):
                forced = safe_next if action == 0 else None
                successors = choose_successors(
                    rng, candidates, successors_per_action, forced
                )
                actual_successor_counts.append(len(successors))
                fields = [str(action), str(len(successors))]
                for succ in successors:
                    fields.append(str(succ))
                    fields.append(f"{random_cost(rng, min_cost, max_cost):.6f}")
                out.write(" ".join(fields) + "\n")

    return {
        "min_actual_s": min(actual_successor_counts) if actual_successor_counts else 0,
        "max_actual_s": max(actual_successor_counts) if actual_successor_counts else 0,
        "avg_actual_s": (
            sum(actual_successor_counts) / len(actual_successor_counts)
            if actual_successor_counts else 0.0
        ),
    }


def main() -> None:
    args = parse_args()
    if args.actions <= 0:
        raise ValueError("--actions must be positive")
    if args.cases <= 0:
        raise ValueError("--cases must be positive")
    if len(set(args.sizes)) != len(args.sizes):
        raise ValueError("--sizes must contain unique values")
    if (
        not math.isfinite(args.min_cost)
        or not math.isfinite(args.max_cost)
        or args.min_cost < 0
        or args.max_cost < args.min_cost
    ):
        raise ValueError("cost range must be finite, non-negative, and ordered")
    for n in args.sizes:
        if n < 3:
            raise ValueError("all graph sizes must be at least 3")

    successor_values: list[int]
    if args.successors_values is not None:
        successor_values = list(args.successors_values)
    else:
        successor_values = [args.successors]

    for s in successor_values:
        if s <= 0:
            raise ValueError("successor count must be positive")
    if len(set(successor_values)) != len(successor_values):
        raise ValueError("--successors-values must contain unique values")

    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    metadata_rows: list[dict[str, str | int | float]] = []

    generated = 0
    for s in successor_values:
        for n in args.sizes:
            for case in range(args.cases):
                graph_seed = args.seed + case
                rng_seed = args.seed * 1_000_003 + n * 9_176 + case
                rng = random.Random(rng_seed)
                path = output / (
                    f"medium_n{n}_s{s}_a{args.actions}_"
                    f"case{case}_seed{graph_seed}.txt"
                )
                stats = write_graph(
                    path,
                    n,
                    rng,
                    args.actions,
                    s,
                    args.min_cost,
                    args.max_cost,
                )
                metadata_rows.append(
                    {
                        "graph_id": path.stem,
                        "n": n,
                        "actions": args.actions,
                        "case": case,
                        "base_seed": args.seed,
                        "display_seed": graph_seed,
                        "rng_seed": rng_seed,
                        "requested_s": s,
                        "min_actual_s": stats["min_actual_s"],
                        "max_actual_s": stats["max_actual_s"],
                        "avg_actual_s": round(stats["avg_actual_s"], 6),
                        "min_cost": args.min_cost,
                        "max_cost": args.max_cost,
                    }
                )
                generated += 1

    metadata_suffix = (
        "_".join(str(s) for s in successor_values)
        if len(successor_values) > 1
        else str(successor_values[0])
    )
    metadata_path = output / f"graph_metadata_s{metadata_suffix}.csv"
    combined_metadata_path = output / "graph_metadata.csv"
    with metadata_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "graph_id",
                "n",
                "actions",
                "case",
                "base_seed",
                "display_seed",
                "rng_seed",
                "requested_s",
                "min_actual_s",
                "max_actual_s",
                "avg_actual_s",
                "min_cost",
                "max_cost",
            ],
        )
        writer.writeheader()
        writer.writerows(metadata_rows)
    with combined_metadata_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "graph_id",
                "n",
                "actions",
                "case",
                "base_seed",
                "display_seed",
                "rng_seed",
                "requested_s",
                "min_actual_s",
                "max_actual_s",
                "avg_actual_s",
                "min_cost",
                "max_cost",
            ],
        )
        writer.writeheader()
        writer.writerows(metadata_rows)

    print(f"generated {generated} graphs in {output}")
    print(f"wrote metadata to {metadata_path}")
    print(f"wrote combined metadata to {combined_metadata_path}")


if __name__ == "__main__":
    main()
