#!/usr/bin/env python3
"""Paired per-graph comparison of VI vs each deterministic baseline from an
Exp4 robustness.csv. For every graph we compare VI's worst_cost to each
baseline's worst_cost (an invalid/improper baseline counts as +inf), and report
the fraction of graphs on which VI is no worse, grouped by s."""
import csv
import sys
import collections

rows = list(csv.DictReader(open(sys.argv[1])))
by_graph = collections.defaultdict(dict)
s_of = {}
for r in rows:
    raw = r["worst_cost"].strip().lower()
    wc = float("inf") if raw == "inf" else float(r["worst_cost"])
    valid = int(r["policy_valid"])
    eff = wc if valid == 1 else float("inf")
    by_graph[r["graph_id"]][r["policy_type"]] = eff
    s_of[r["graph_id"]] = int(r["s"])

baselines = ["baseline_nominal", "baseline_bestcase", "baseline_worst_immediate"]
per_s = collections.defaultdict(lambda: {b: [0, 0] for b in baselines})
total = {b: [0, 0] for b in baselines}

for gid, pol in by_graph.items():
    if "vi" not in pol:
        continue
    vi = pol["vi"]
    s = s_of[gid]
    for b in baselines:
        if b not in pol:
            continue
        win = 1 if vi <= pol[b] + 1e-9 else 0
        per_s[s][b][0] += win
        per_s[s][b][1] += 1
        total[b][0] += win
        total[b][1] += 1

print("VI no-worse-than-baseline win count / total graphs, by s:")
for s in sorted(per_s):
    parts = []
    for b in baselines:
        won, tot = per_s[s][b]
        parts.append(b.replace("baseline_", "") + "=" + str(won) + "/" + str(tot))
    print("  s=" + str(s) + ": " + ", ".join(parts))
print("overall:")
for b in baselines:
    won, tot = total[b]
    print("  vi vs " + b + ": " + str(won) + "/" + str(tot))
