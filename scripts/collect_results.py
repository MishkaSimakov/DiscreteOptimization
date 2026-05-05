#!/usr/bin/env python3
import json
import os
import glob
from datetime import datetime, timezone

TASK_PREFIXES = {
    "sc_": "setcover",
    "ks_": "knapsack",
    "gc_": "coloring",
    "tsp_": "tsp",
    "fl_": "facility",
    "vrp_": "vrp",
}


def get_task(instance_name):
    for prefix, task in TASK_PREFIXES.items():
        if instance_name.startswith(prefix):
            return task
    return "unknown"


def main():
    artifacts_dir = os.environ.get("ARTIFACTS_DIR", "artifacts")
    output_file = os.environ.get("OUTPUT_FILE", "dashboard-data/history.jsonl")
    commit = os.environ.get("COMMIT", "unknown")
    branch = os.environ.get("BRANCH", "unknown")

    results = {}
    for stats_file in sorted(glob.glob(f"{artifacts_dir}/*_stats.json")):
        instance = os.path.basename(stats_file)[: -len("_stats.json")]
        task = get_task(instance)
        try:
            with open(stats_file) as f:
                data = json.load(f)
            results[f"{task}/{instance}"] = {
                "score": data["score"],
                "duration": data["duration"],
            }
        except (json.JSONDecodeError, KeyError) as e:
            print(f"Warning: could not parse {stats_file}: {e}")

    if not results:
        print("No results found, nothing to append.")
        return

    record = {
        "commit": commit,
        "branch": branch,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "results": results,
    }

    with open(output_file, "a") as f:
        f.write(json.dumps(record) + "\n")

    print(f"Appended {len(results)} results for commit {commit[:7]} ({branch})")


if __name__ == "__main__":
    main()
