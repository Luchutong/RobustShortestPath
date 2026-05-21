#!/usr/bin/env python3
"""Generate layered medium-size RSP graphs for runtime experiments."""

from __future__ import annotations

import argparse
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
    successors: list[int] = []
    if forced is not None:
        successors.append(forced)
    while len(successors) < count:
        if len(candidates) >= count:
            candidate = rng.choice(candidates)
            if candidate in successors:
                continue
            successors.append(candidate)
        else:
            successors.append(rng.choice(candidates))
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
) -> None:
    terminal = n - 1
    layers = build_layers(n)
    node_layer = {
        node: layer_index
        for layer_index, layer in enumerate(layers)
        for node in layer
    }

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
                fields = [str(action), str(len(successors))]
                for succ in successors:
                    fields.append(str(succ))
                    fields.append(f"{random_cost(rng, min_cost, max_cost):.6f}")
                out.write(" ".join(fields) + "\n")


def main() -> None:
    args = parse_args()
    if args.actions <= 0:
        raise ValueError("--actions must be positive")
    if args.successors <= 0:
        raise ValueError("--successors must be positive")
    if args.min_cost < 0 or args.max_cost < args.min_cost:
        raise ValueError("cost range must be non-negative and ordered")
    for n in args.sizes:
        if n < 3:
            raise ValueError("all graph sizes must be at least 3")

    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)

    generated = 0
    for n in args.sizes:
        for case in range(args.cases):
            graph_seed = args.seed + case
            rng = random.Random(args.seed * 1_000_003 + n * 9_176 + case)
            path = output / f"medium_n{n}_seed{graph_seed}.txt"
            write_graph(
                path,
                n,
                rng,
                args.actions,
                args.successors,
                args.min_cost,
                args.max_cost,
            )
            generated += 1

    print(f"generated {generated} graphs in {output}")


if __name__ == "__main__":
    main()
