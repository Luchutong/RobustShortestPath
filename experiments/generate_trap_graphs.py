#!/usr/bin/env python3
"""Generate "trap" RSP graphs where a robust (proper) policy exists but the
deterministic baselines are lured into a policy that is improper under the
adversary.

Motivation: the layered-DAG family in generate_medium_graphs.py makes EVERY
policy proper by construction, so Exp4 there shows valid_rate=1.0 for all
policies -- the robustness advantage shows up only in worst-case cost, never in
a baseline actually FAILING. These graphs exercise the failure mode: the nominal
/ best-case baselines optimise the cheap nominal successor and pick a "risky"
action whose adversarial successor closes a cycle, so their policy is improper
(policy_valid=0), while value iteration picks the safe action and stays proper.

Gadget (3 nodes: u=0, v=1, terminal=2):
  u: a0 risky  -> [terminal(cost cheap_t), v(cost cheap_v)]   # nominal sees u->T
     a1 safe   -> [terminal(cost safe_cost)]                  # always reaches T
  v: b0        -> [terminal(cost gamma), u(cost delta)]        # adversary can send v->u

A proper policy always exists ({u=a1, v=b0}); the nominal/best-case baseline
picks a0 whenever cheap_t < safe_cost (the common case), giving the cyclic
u<->v policy that is improper under the adversary.
"""

from __future__ import annotations

import argparse
import csv
import random
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate trap RSP graphs that make deterministic baselines fail."
    )
    parser.add_argument("--output", default="data/trap_graphs")
    parser.add_argument("--cases", type=int, default=20)
    parser.add_argument("--seed", type=int, default=42)
    return parser.parse_args()


def write_graph(path: Path, rng: random.Random) -> dict[str, float]:
    cheap_t = round(rng.uniform(1.0, 5.0), 6)
    cheap_v = round(rng.uniform(1.0, 5.0), 6)
    safe_cost = round(rng.uniform(1.0, 30.0), 6)
    gamma = round(rng.uniform(1.0, 5.0), 6)
    delta = round(rng.uniform(1.0, 5.0), 6)

    with path.open("w", encoding="utf-8") as out:
        out.write("3 2\n")
        # node 0 (u): risky action 0, safe action 1
        out.write("2\n")
        out.write(f"0 2 2 {cheap_t:.6f} 1 {cheap_v:.6f}\n")
        out.write(f"1 1 2 {safe_cost:.6f}\n")
        # node 1 (v): single action that the adversary can route back to u
        out.write("1\n")
        out.write(f"0 2 2 {gamma:.6f} 0 {delta:.6f}\n")
        # node 2: terminal
        out.write("0\n")

    # nominal baseline takes the risky shortcut (and becomes improper) whenever
    # the nominal path through a0 is cheaper than the safe action.
    nominal_takes_trap = cheap_t < safe_cost
    return {
        "cheap_t": cheap_t,
        "cheap_v": cheap_v,
        "safe_cost": safe_cost,
        "gamma": gamma,
        "delta": delta,
        "nominal_takes_trap": int(nominal_takes_trap),
    }


def main() -> None:
    args = parse_args()
    if args.cases <= 0:
        raise ValueError("--cases must be positive")

    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    metadata_rows: list[dict[str, object]] = []

    for case in range(args.cases):
        rng_seed = args.seed * 1_000_003 + case
        rng = random.Random(rng_seed)
        graph_id = f"trap_case{case}_seed{args.seed + case}"
        path = output / f"{graph_id}.txt"
        stats = write_graph(path, rng)
        row: dict[str, object] = {"graph_id": graph_id, "case": case,
                                  "base_seed": args.seed, "rng_seed": rng_seed}
        row.update(stats)
        metadata_rows.append(row)

    metadata_path = output / "trap_metadata.csv"
    with metadata_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "graph_id", "case", "base_seed", "rng_seed",
                "cheap_t", "cheap_v", "safe_cost", "gamma", "delta",
                "nominal_takes_trap",
            ],
        )
        writer.writeheader()
        writer.writerows(metadata_rows)

    trapped = sum(int(r["nominal_takes_trap"]) for r in metadata_rows)
    print(f"generated {args.cases} trap graphs in {output}")
    print(f"nominal baseline lured into the trap on {trapped}/{args.cases} graphs")
    print(f"wrote metadata to {metadata_path}")


if __name__ == "__main__":
    main()
