#!/usr/bin/env python3
"""Compile and run the Day 5 C++20 parallelism lab."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def compiler() -> str:
    requested = os.environ.get("CXX")
    if requested:
        resolved = shutil.which(requested)
        if resolved:
            return resolved
        raise SystemExit(f"CXX names an unavailable compiler: {requested}")
    for candidate in ("clang++", "g++", "c++"):
        resolved = shutil.which(candidate)
        if resolved:
            return resolved
    raise SystemExit("No C++ compiler found. Run scripts/bootstrap.sh first.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=Path("labs/parallelism/answer.cpp"))
    parser.add_argument("--items", type=int, default=4_000_000)
    parser.add_argument("--counter-iterations", type=int, default=750_000)
    args = parser.parse_args()

    source = args.source.resolve()
    if not source.is_file():
        raise SystemExit(
            f"Missing {source}. Copy labs/parallelism/starter.cpp to "
            "labs/parallelism/answer.cpp and implement the TODOs."
        )
    if args.items < 1 or args.counter_iterations < 1:
        raise SystemExit("--items and --counter-iterations must be positive")

    root = Path(__file__).resolve().parents[2]
    output_directory = root / "build" / "parallelism"
    output_directory.mkdir(parents=True, exist_ok=True)
    executable = output_directory / ("parallelism.exe" if os.name == "nt" else "parallelism")
    command = [
        compiler(),
        "-std=c++20",
        "-O2",
        "-pthread",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
        str(source),
        "-o",
        str(executable),
    ]
    print("[compile]", " ".join(command), flush=True)
    subprocess.run(command, cwd=root, check=True)
    print("[run] correctness + scaling + cache-line experiment", flush=True)
    completed = subprocess.run(
        [str(executable), str(args.items), str(args.counter_iterations)],
        cwd=root,
        check=False,
        timeout=60,
    )
    return completed.returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.TimeoutExpired:
        print("Lab exceeded 60 seconds; reduce inputs or diagnose the implementation.", file=sys.stderr)
        raise SystemExit(124)

