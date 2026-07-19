#!/usr/bin/env python3
"""Compile and run one Day 10 sanitizer fixture with an explicit expectation."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


def find_clang() -> str:
    requested = os.environ.get("CXX")
    if requested:
        resolved = shutil.which(requested)
        if resolved and "clang" in Path(resolved).name:
            return resolved
    resolved = shutil.which("clang++")
    if resolved:
        return resolved
    raise SystemExit("clang++ is required. Run scripts/bootstrap.sh and scripts/doctor.sh.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", choices=("asan", "tsan"), required=True)
    parser.add_argument("--expect", choices=("failure", "clean"), required=True)
    parser.add_argument("--source", type=Path)
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    source = args.source or Path(f"labs/debugging/{args.case}-answer.cpp")
    source = (root / source).resolve() if not source.is_absolute() else source.resolve()
    if not source.is_file():
        raise SystemExit(
            f"Missing {source}. Copy labs/debugging/{args.case}-starter.cpp to "
            f"labs/debugging/{args.case}-answer.cpp first."
        )

    output_directory = root / "build" / "debugging"
    output_directory.mkdir(parents=True, exist_ok=True)
    executable = output_directory / args.case
    sanitizer = "address,undefined" if args.case == "asan" else "thread"
    compile_command = [
        find_clang(),
        "-std=c++20",
        "-O1",
        "-g",
        "-fno-omit-frame-pointer",
        f"-fsanitize={sanitizer}",
        "-pthread",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        str(source),
        "-o",
        str(executable),
    ]
    print("[compile]", " ".join(compile_command), flush=True)
    compiled = subprocess.run(compile_command, cwd=root, text=True, capture_output=True)
    if compiled.stdout:
        print(compiled.stdout, end="")
    if compiled.stderr:
        print(compiled.stderr, end="", file=sys.stderr)
    if compiled.returncode != 0:
        print("Compilation failed; this is not a sanitizer result.", file=sys.stderr)
        return 2

    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "halt_on_error=1:detect_leaks=1"
    environment["TSAN_OPTIONS"] = "halt_on_error=1"
    print(f"[run] expecting {args.expect} from {args.case}", flush=True)
    result = subprocess.run(
        [str(executable)],
        cwd=root,
        env=environment,
        text=True,
        capture_output=True,
        timeout=20,
    )
    combined = (result.stdout or "") + (result.stderr or "")
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    signature = "AddressSanitizer" if args.case == "asan" else "ThreadSanitizer"
    reported = signature in combined

    if args.expect == "failure":
        matched = result.returncode != 0 and reported
    else:
        matched = result.returncode == 0 and not reported
    print(
        f"[expectation] {'PASS' if matched else 'FAIL'}: "
        f"exit={result.returncode}, {signature} report={'yes' if reported else 'no'}"
    )
    return 0 if matched else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.TimeoutExpired:
        print("Fixture exceeded 20 seconds; diagnose the program instead of extending the timeout.", file=sys.stderr)
        raise SystemExit(124)

