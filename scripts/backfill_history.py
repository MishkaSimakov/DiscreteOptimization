#!/usr/bin/env python3
"""
Download historical CI results from GitHub Actions artifacts and append to
dashboard-data/history.jsonl.

Run once from the repo root (requires gh CLI, authenticated):

    python3 scripts/backfill_history.py [--dry-run] [--no-push]

Resume-safe: commits already recorded in history.jsonl are skipped.
Expired artifacts are skipped silently.
"""

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

TASK_PREFIXES = {
    "sc_": "setcover",
    "ks_": "knapsack",
    "gc_": "coloring",
    "tsp_": "tsp",
    "fl_": "facility",
    "vrp_": "vrp"
}


def get_task(instance: str) -> str:
    for prefix, task in TASK_PREFIXES.items():
        if instance.startswith(prefix):
            return task
    return "unknown"


def gh(*args, check=True) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["gh", *args], capture_output=True, text=True, cwd=REPO_ROOT,
        check=check,
    )


def git(*args, cwd=None, check=True) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["git", *args], capture_output=True, text=True,
        cwd=cwd or REPO_ROOT, check=check,
    )


def load_processed(history_path: Path) -> set[str]:
    if not history_path.exists():
        return set()
    seen = set()
    for line in history_path.read_text().splitlines():
        line = line.strip()
        if line:
            try:
                seen.add(json.loads(line)["commit"])
            except (json.JSONDecodeError, KeyError):
                pass
    return seen


def list_runs() -> list[dict]:
    result = gh(
        "run", "list",
        "--workflow", "grade.yml",
        "--limit", "200",
        "--json", "databaseId,headSha,headBranch,createdAt",
    )
    return json.loads(result.stdout)


def download_artifact(run_id: int, dest: Path) -> bool:
    result = subprocess.run(
        ["gh", "run", "download", str(run_id), "--name", "all-results", "--dir", str(dest)],
        capture_output=True, text=True, cwd=REPO_ROOT,
    )
    return result.returncode == 0


def parse_stats(artifact_dir: Path) -> dict:
    results = {}
    for stats_file in sorted(artifact_dir.rglob("*_stats.json")):
        instance = stats_file.name[: -len("_stats.json")]
        task = get_task(instance)
        try:
            data = json.loads(stats_file.read_text())
            results[f"{task}/{instance}"] = {
                "score": data["score"],
                "duration": data["duration"],
            }
        except (json.JSONDecodeError, KeyError):
            pass
    return results


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--dry-run", action="store_true",
                        help="Show plan without writing anything")
    parser.add_argument("--no-push", action="store_true",
                        help="Skip pushing to remote at the end")
    args = parser.parse_args()

    dashboard_dir = Path(tempfile.mkdtemp(prefix="dashboard-data-"))
    try:
        git("worktree", "add", str(dashboard_dir), "dashboard-data")
        history_path = dashboard_dir / "history.jsonl"

        processed = load_processed(history_path)
        print(f"Already in history.jsonl: {len(processed)} commit(s)")

        print("Fetching workflow run list...")
        runs = list_runs()
        print(f"Total runs found: {len(runs)}")

        # Sort chronologically, deduplicate by SHA (first run per commit wins)
        runs.sort(key=lambda r: r["createdAt"])
        seen_shas: set[str] = set(processed)
        to_process = []
        for run in runs:
            sha = run["headSha"]
            if sha not in seen_shas:
                seen_shas.add(sha)
                to_process.append(run)

        print(f"Runs to process: {len(to_process)}\n")
        for run in to_process:
            print(f"  {run['headSha'][:7]}  {run['createdAt'][:10]}  [{run['headBranch']}]  id={run['databaseId']}")

        if args.dry_run or not to_process:
            return

        print()
        success = 0

        for i, run in enumerate(to_process, 1):
            sha = run["headSha"]
            branch = run["headBranch"]
            ts = run["createdAt"]
            print(f"[{i}/{len(to_process)}] {sha[:7]}  {ts[:10]}  [{branch}]", end="  ", flush=True)

            tmpdir = Path(tempfile.mkdtemp())
            try:
                if not download_artifact(run["databaseId"], tmpdir):
                    print("no artifact (expired or missing) — skipped")
                    continue

                results = parse_stats(tmpdir)
                if not results:
                    print("no stats files — skipped")
                    continue

                record = {
                    "commit": sha,
                    "branch": branch,
                    "timestamp": ts,
                    "results": results,
                }
                with open(history_path, "a") as f:
                    f.write(json.dumps(record) + "\n")

                print(f"{len(results)} instance(s) recorded")
                success += 1
            finally:
                shutil.rmtree(tmpdir, ignore_errors=True)

        print(f"\nDone. Recorded {success}/{len(to_process)} run(s).")

        if args.no_push or success == 0:
            return

        git("config", "user.email", "backfill@local", cwd=dashboard_dir)
        git("config", "user.name", "Backfill Script", cwd=dashboard_dir)
        git("add", "history.jsonl", cwd=dashboard_dir)
        diff = git("diff", "--cached", "--quiet", cwd=dashboard_dir, check=False)
        if diff.returncode == 0:
            print("Nothing to push.")
            return
        git("commit", "-m", "backfill: historical results", cwd=dashboard_dir)
        git("pull", "--rebase", "origin", "dashboard-data", cwd=dashboard_dir)
        git("push", "origin", "dashboard-data", cwd=dashboard_dir)
        print("Pushed history.jsonl to dashboard-data.")

    except KeyboardInterrupt:
        print("\n\nInterrupted — re-run to continue from where you left off.")
        sys.exit(1)
    finally:
        git("worktree", "remove", "--force", str(dashboard_dir), check=False)
        shutil.rmtree(dashboard_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
