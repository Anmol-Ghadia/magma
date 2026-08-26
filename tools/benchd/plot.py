#!/usr/bin/env python3
"""
plot_bugs.py

Plots a grouped bar chart from combined experiment summary data of the shape:

  results -> <fuzzer> -> <target> -> <prefix> -> <trial_id> -> {"reached": {...}, "triggered": {...}}

(i.e. the output of get_experiment_summary / combine_by_prefix, where "prefix"
groups fuzz drivers by the tool that generated them, e.g. "promefuzz", "opencode".)

Every bug key found anywhere in the data (for the chosen fuzzer/target) becomes
one x-axis group automatically -- no need to list them. Within a group, each
prefix contributes two adjacent bars: "reached" and "triggered". Bar color
identifies the prefix; bar fill/hatch identifies the metric (solid = reached,
hatched = triggered). Bar height is the mean across trials; the error bar
spans [min, max] across trials. Y axis is log scale.

Usage:
    python plot_bugs.py combined.json --output bugs.png
    python plot_bugs.py combined.json --fuzzer aflplusplus --target libtiff -o out.png
"""

import argparse
import json
import statistics as stats

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

METRICS = ["reached", "triggered"]
METRIC_HATCH = {"reached": "", "triggered": "//"}


def discover_bugs(prefixes_data):
    """
    Union of every bug id seen across all prefixes, trials, and both metrics
    for the selected fuzzer/target, sorted for a stable x-axis order.
    """
    bugs = set()
    for trials in prefixes_data.values():
        for trial in trials.values():
            for metric in METRICS:
                bugs.update(trial.get(metric, {}).keys())
    return sorted(bugs)


def collect_values(prefixes_data, bug, metric):
    """
    Return {prefix: [value_per_trial, ...]} for a given bug/metric.
    A bug missing from a trial's dict counts as 0 (i.e. not reached/triggered).
    """
    out = {}
    for prefix, trials in prefixes_data.items():
        out[prefix] = [trial.get(metric, {}).get(bug, 0) for trial in trials.values()]
    return out


def pick_fuzzer_and_target(data, fuzzer_arg, target_arg):
    results = data["results"]

    if fuzzer_arg is None:
        fuzzer_arg = next(iter(results))
        print(f"No --fuzzer given, using '{fuzzer_arg}'")
    if fuzzer_arg not in results:
        raise SystemExit(f"Fuzzer '{fuzzer_arg}' not found. Available: {list(results)}")

    targets = results[fuzzer_arg]
    if target_arg is None:
        target_arg = next(iter(targets))
        print(f"No --target given, using '{target_arg}'")
    if target_arg not in targets:
        raise SystemExit(f"Target '{target_arg}' not found. Available: {list(targets)}")

    return fuzzer_arg, target_arg


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", help="path to combined summary JSON file")
    parser.add_argument("--fuzzer", default=None, help="fuzzer name (default: first one found)")
    parser.add_argument("--target", default=None, help="target name (default: first one found)")
    parser.add_argument("-o", "--output", default="bug_plot.png", help="output image path")
    args = parser.parse_args()

    with open(args.input) as f:
        data = json.load(f)

    fuzzer, target = pick_fuzzer_and_target(data, args.fuzzer, args.target)
    prefixes_data = data["results"][fuzzer][target]

    bugs = discover_bugs(prefixes_data)
    if not bugs:
        raise SystemExit("No bug keys found in the data for this fuzzer/target.")

    prefixes = list(prefixes_data.keys())

    n_groups = len(bugs)
    n_bars = len(prefixes) * len(METRICS)
    bar_width = 0.8 / max(n_bars, 1)
    x = np.arange(n_groups)

    fig, ax = plt.subplots(figsize=(1.5 + 1.2 * n_groups, 5.5))

    color_cycle = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    prefix_colors = {prefix: color_cycle[i % len(color_cycle)] for i, prefix in enumerate(prefixes)}

    # bar_index tracks position within the group across all (prefix, metric) pairs
    bar_index = 0
    for prefix in prefixes:
        for metric in METRICS:
            means, lower_err, upper_err = [], [], []
            for bug in bugs:
                values = collect_values(prefixes_data, bug, metric)[prefix]
                if not values:
                    means.append(0)
                    lower_err.append(0)
                    upper_err.append(0)
                    continue
                mean_val = stats.mean(values)
                means.append(mean_val)
                lower_err.append(mean_val - min(values))
                upper_err.append(max(values) - mean_val)

            offset = (bar_index - (n_bars - 1) / 2) * bar_width
            ax.bar(
                x + offset, means, width=bar_width,
                color=prefix_colors[prefix], hatch=METRIC_HATCH[metric],
                edgecolor="black", linewidth=0.5,
                yerr=[lower_err, upper_err], capsize=3, error_kw={"linewidth": 1},
            )
            bar_index += 1
    ax.axhline(1270, color=prefix_colors['promefuzz'], linestyle="--", linewidth=1, label="back-computed time per harness for promefuzz")
    ax.axhline(14400, color=prefix_colors['opencode'], linestyle="--", linewidth=1, label="back-computed time per harness for opencode")

    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(bugs, rotation=45, ha="right")
    ax.set_xlabel("Bug ID")
    ax.set_ylabel("count (log scale)")
    ax.set_title(f"Reached / triggered counts — {fuzzer} / {target}\n(bar = mean across trials, error bars = min/max)")

    # two separate legends: color -> prefix, hatch -> metric
    prefix_handles = [mpatches.Patch(facecolor=prefix_colors[p], edgecolor="black", label=p) for p in prefixes]
    metric_handles = [
        mpatches.Patch(facecolor="white", edgecolor="black", hatch=METRIC_HATCH[m], label=m) for m in METRICS
    ]
    legend1 = ax.legend(handles=prefix_handles, title="prefix", loc="upper left", bbox_to_anchor=(1.01, 1.0))
    ax.add_artist(legend1)
    ax.legend(handles=metric_handles, title="metric", loc="lower left", bbox_to_anchor=(1.01, 0.0))

    fig.tight_layout()
    fig.savefig(args.output, dpi=150, bbox_inches="tight")
    print(f"Saved plot to {args.output}")


if __name__ == "__main__":
    main()
