#!/usr/bin/env python3
"""Deterministic POSIX runner for the closed-book line-reversal screen."""

from __future__ import annotations

import csv
import hashlib
import os
from pathlib import Path
import platform
import shlex
import shutil
import signal
import statistics
import subprocess
import sys
import tempfile
import time


TIMEOUT_SECONDS = 20.0
LARGE_BLOCK = b"0123456789abcdef"
LARGE_REPETITIONS = 1_048_576  # 16 MiB before the CRLF terminator.


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_or_missing(path: Path) -> str:
    return sha256_file(path) if path.is_file() else "<missing>"


def files_equal(left: Path, right: Path) -> bool:
    if not left.exists() or not right.exists() or left.stat().st_size != right.stat().st_size:
        return False
    with left.open("rb") as lhs, right.open("rb") as rhs:
        while True:
            left_chunk = lhs.read(1024 * 1024)
            right_chunk = rhs.read(1024 * 1024)
            if left_chunk != right_chunk:
                return False
            if not left_chunk:
                return True


def write_bytes(path: Path, value: bytes) -> None:
    path.write_bytes(value)


def write_repeated(path: Path, block: bytes, count: int, suffix: bytes = b"") -> None:
    with path.open("wb") as stream:
        batch_count = 4096
        batch = block * batch_count
        whole_batches, remainder = divmod(count, batch_count)
        for _ in range(whole_batches):
            stream.write(batch)
        stream.write(block * remainder)
        stream.write(suffix)


def create_fixtures(root: Path) -> list[tuple[str, Path, Path]]:
    cases: list[tuple[str, Path, Path]] = []

    def add(name: str, source: bytes, expected: bytes) -> None:
        input_path = root / f"{name}.in"
        expected_path = root / f"{name}.expected"
        write_bytes(input_path, source)
        write_bytes(expected_path, expected)
        cases.append((name, input_path, expected_path))

    add("empty", b"", b"")
    add("lf-terminated", b"alpha\nbeta\n", b"ahpla\nateb\n")
    add("final-unterminated", b"alpha\nbeta", b"ahpla\nateb")
    add("empty-lines", b"\n\nx\n", b"\n\nx\n")
    add(
        "mixed-newlines",
        b"alpha\r\nbeta\n\r\nomega\r\nlast",
        b"ahpla\r\nateb\n\r\nagemo\r\ntsal",
    )
    add("binary-bytes", b"\x00\x01\xff\nA\x00B", b"\xff\x01\x00\nB\x00A")

    many_input = root / "many-short-lines.in"
    many_expected = root / "many-short-lines.expected"
    with many_input.open("wb") as source, many_expected.open("wb") as expected:
        for index in range(50_000):
            content = f"line-{index:06d}-{(index * 2_654_435_761) & 0xFFFFFFFF:08x}".encode()
            terminator = b"\r\n" if index % 11 == 0 else b"\n"
            source.write(content + terminator)
            expected.write(content[::-1] + terminator)
    cases.append(("many-short-lines", many_input, many_expected))

    large_input = root / "large-line-16mib.in"
    large_expected = root / "large-line-16mib.expected"
    write_repeated(large_input, LARGE_BLOCK, LARGE_REPETITIONS, b"\r\n")
    write_repeated(large_expected, LARGE_BLOCK[::-1], LARGE_REPETITIONS, b"\r\n")
    cases.append(("large-line-16mib", large_input, large_expected))
    return cases


def run_child(command: list[str], stdout_path: Path, stderr_path: Path,
              timeout_seconds: float = TIMEOUT_SECONDS) -> tuple[int, float, int, bool]:
    if not hasattr(os, "fork") or not hasattr(os, "wait4"):
        raise RuntimeError("this runner requires Linux, WSL, or macOS (fork/wait4)")

    started = time.perf_counter()
    pid = os.fork()
    if pid == 0:
        try:
            stdout_fd = os.open(stdout_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
            stderr_fd = os.open(stderr_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
            os.dup2(stdout_fd, 1)
            os.dup2(stderr_fd, 2)
            os.close(stdout_fd)
            os.close(stderr_fd)
            os.execv(command[0], command)
        except BaseException as error:  # Child must report exec failures without traceback machinery.
            os.write(2, f"runner exec failure: {error}\n".encode())
            os._exit(127)

    deadline = started + timeout_seconds
    timed_out = False
    while True:
        completed_pid, status, usage = os.wait4(pid, os.WNOHANG)
        if completed_pid == pid:
            break
        if time.perf_counter() >= deadline:
            timed_out = True
            os.kill(pid, signal.SIGKILL)
            _, status, usage = os.wait4(pid, 0)
            break
        time.sleep(0.01)

    elapsed_ms = (time.perf_counter() - started) * 1000.0
    peak_rss_kib = int(usage.ru_maxrss / 1024) if sys.platform == "darwin" else int(usage.ru_maxrss)
    return os.waitstatus_to_exitcode(status), elapsed_ms, peak_rss_kib, timed_out


def compile_candidate(source: Path, executable: Path, evidence: Path) -> tuple[bool, str]:
    compiler = os.environ.get("CXX", "c++")
    compiler_path = shutil.which(compiler)
    if compiler_path is None:
        raise RuntimeError(f"compiler not found: {compiler!r}; set CXX or install c++")
    command = [
        compiler_path,
        "-std=c++20",
        "-O2",
        "-g",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        str(source),
        "-o",
        str(executable),
    ]
    completed = subprocess.run(command, capture_output=True, text=True, timeout=60, check=False)
    log = "command: " + shlex.join(command) + "\n\n" + completed.stdout + completed.stderr
    (evidence / "compile.txt").write_text(log, encoding="utf-8")
    return completed.returncode == 0, compiler_path


def result_row(name: str, passed: bool, exit_code: int, input_bytes: int,
               output_bytes: int, elapsed_ms: float, peak_rss_kib: int,
               detail: str) -> dict[str, object]:
    return {
        "case": name,
        "result": "PASS" if passed else "FAIL",
        "exit_code": exit_code,
        "input_bytes": input_bytes,
        "output_bytes": output_bytes,
        "elapsed_ms": f"{elapsed_ms:.3f}",
        "peak_rss_kib": peak_rss_kib,
        "detail": detail,
    }


def write_tsv(path: Path, rows: list[dict[str, object]]) -> None:
    fields = [
        "case", "result", "exit_code", "input_bytes", "output_bytes",
        "elapsed_ms", "peak_rss_kib", "detail",
    ]
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    repository_root = script_dir.parent.parent
    source = Path(sys.argv[1]).resolve() if len(sys.argv) >= 2 else script_dir / "starter.cpp"
    evidence = (
        Path(sys.argv[2]).resolve()
        if len(sys.argv) >= 3
        else repository_root / "capstone/work/evidence/day-14/quantiq-line-reversal"
    )
    if len(sys.argv) > 3:
        print(f"usage: {Path(sys.argv[0]).name} [SOURCE.cpp] [EVIDENCE_DIR]", file=sys.stderr)
        return 2
    if not source.is_file():
        print(f"source file does not exist: {source}", file=sys.stderr)
        return 2

    evidence.mkdir(parents=True, exist_ok=True)
    logs = evidence / "logs"
    logs.mkdir(exist_ok=True)
    for old_log in logs.glob("*.txt"):
        old_log.unlink()

    with tempfile.TemporaryDirectory(prefix="netforge-line-screen-") as temporary:
        temporary_root = Path(temporary)
        executable = (temporary_root / "reverse-lines").resolve()
        try:
            compiled, compiler = compile_candidate(source, executable, evidence)
        except (OSError, RuntimeError, subprocess.SubprocessError) as error:
            (evidence / "compile.txt").write_text(str(error) + "\n", encoding="utf-8")
            print(f"compile setup failed: {error}", file=sys.stderr)
            return 2

        compiler_version = subprocess.run(
            [compiler, "--version"], capture_output=True, text=True, check=False
        ).stdout.splitlines()
        environment_lines = [
            f"platform={platform.platform()}",
            f"python={platform.python_version()}",
            f"compiler={compiler_version[0] if compiler_version else compiler}",
            f"source={source}",
            f"source_sha256={sha256_file(source)}",
            "runner_command=" + shlex.join([sys.executable, *sys.argv]),
            "measurement=wall clock via perf_counter; child peak RSS via wait4",
            "peak_rss_unit=KiB (macOS ru_maxrss bytes normalized to KiB)",
        ]
        (evidence / "environment.txt").write_text("\n".join(environment_lines) + "\n", encoding="utf-8")

        if not compiled:
            (evidence / "summary.md").write_text(
                "# Line-reversal screen result\n\nCompilation failed. See `compile.txt`.\n",
                encoding="utf-8",
            )
            print(f"FAIL: compilation failed; see {evidence / 'compile.txt'}")
            return 1

        fixture_root = temporary_root / "fixtures"
        fixture_root.mkdir()
        fixtures = create_fixtures(fixture_root)
        rows: list[dict[str, object]] = []

        for name, input_path, expected_path in fixtures:
            output_path = temporary_root / f"{name}.out"
            output_path.write_bytes(b"stale output that must be truncated")
            exit_code, elapsed_ms, peak_rss_kib, timed_out = run_child(
                [str(executable), str(input_path), str(output_path)],
                logs / f"{name}.stdout.txt",
                logs / f"{name}.stderr.txt",
            )
            equal = files_equal(output_path, expected_path)
            passed = exit_code == 0 and not timed_out and equal
            if timed_out:
                detail = "timed out"
            elif exit_code != 0:
                detail = "valid input returned nonzero"
            elif not equal:
                detail = (
                    f"byte mismatch; expected_sha256={sha256_file(expected_path)} "
                    f"output_sha256={sha256_or_missing(output_path)}"
                )
            else:
                detail = "exact bytes"
            rows.append(result_row(
                name, passed, exit_code, input_path.stat().st_size,
                output_path.stat().st_size if output_path.exists() else 0,
                elapsed_ms, peak_rss_kib, detail,
            ))

        missing_input = temporary_root / "does-not-exist.in"
        missing_output = temporary_root / "missing.out"
        exit_code, elapsed_ms, peak_rss_kib, timed_out = run_child(
            [str(executable), str(missing_input), str(missing_output)],
            logs / "reject-missing.stdout.txt",
            logs / "reject-missing.stderr.txt",
        )
        passed = exit_code != 0 and not timed_out
        rows.append(result_row(
            "reject-missing-input", passed, exit_code, 0,
            missing_output.stat().st_size if missing_output.exists() else 0,
            elapsed_ms, peak_rss_kib,
            "nonzero without timeout" if passed else "missing input was not rejected cleanly",
        ))

        same_path = temporary_root / "same-path.bin"
        same_bytes = b"must remain unchanged\n"
        same_path.write_bytes(same_bytes)
        exit_code, elapsed_ms, peak_rss_kib, timed_out = run_child(
            [str(executable), str(same_path), str(same_path)],
            logs / "reject-same.stdout.txt",
            logs / "reject-same.stderr.txt",
        )
        passed = exit_code != 0 and not timed_out and same_path.read_bytes() == same_bytes
        rows.append(result_row(
            "reject-same-path", passed, exit_code, len(same_bytes), same_path.stat().st_size,
            elapsed_ms, peak_rss_kib,
            "nonzero and input preserved" if passed else "same path was not rejected without modification",
        ))
        write_tsv(evidence / "results.tsv", rows)

        large_passed = next(row for row in rows if row["case"] == "large-line-16mib")["result"] == "PASS"
        performance_rows: list[dict[str, object]] = []
        if large_passed:
            large_input = next(item[1] for item in fixtures if item[0] == "large-line-16mib")
            large_expected = next(item[2] for item in fixtures if item[0] == "large-line-16mib")
            for repetition in range(1, 4):
                output_path = temporary_root / f"performance-{repetition}.out"
                exit_code, elapsed_ms, peak_rss_kib, timed_out = run_child(
                    [str(executable), str(large_input), str(output_path)],
                    logs / f"performance-{repetition}.stdout.txt",
                    logs / f"performance-{repetition}.stderr.txt",
                )
                passed = exit_code == 0 and not timed_out and files_equal(output_path, large_expected)
                performance_rows.append(result_row(
                    f"large-line-run-{repetition}", passed, exit_code,
                    large_input.stat().st_size,
                    output_path.stat().st_size if output_path.exists() else 0,
                    elapsed_ms, peak_rss_kib,
                    "exact bytes" if passed else "performance repetition failed correctness",
                ))
            write_tsv(evidence / "performance.tsv", performance_rows)
        else:
            (evidence / "performance.tsv").write_text(
                "Performance repetitions skipped because the 16 MiB functional case failed.\n",
                encoding="utf-8",
            )

        passed_count = sum(row["result"] == "PASS" for row in rows)
        all_passed = passed_count == len(rows)
        performance_ok = bool(performance_rows) and all(row["result"] == "PASS" for row in performance_rows)
        measurement_note = ""
        if performance_rows:
            elapsed_values = [float(row["elapsed_ms"]) for row in performance_rows]
            rss_values = [int(row["peak_rss_kib"]) for row in performance_rows]
            measurement_note = (
                f"- Large-line median wall time: {statistics.median(elapsed_values):.3f} ms\n"
                f"- Large-line peak RSS range: {min(rss_values)}-{max(rss_values)} KiB\n"
            )
        summary = (
            "# Line-reversal screen result\n\n"
            f"- Functional cases: {passed_count}/{len(rows)} passed\n"
            f"- Three large-line repetitions: {'passed' if performance_ok else 'not completed cleanly'}\n"
            + measurement_note
            + "\nTimes and RSS are environment-specific evidence, not an employer threshold. "
              "Peak RSS includes process/runtime overhead. Review `results.tsv`, `performance.tsv`, "
              "and your written asymptotic memory model together.\n"
        )
        (evidence / "summary.md").write_text(summary, encoding="utf-8")

        for row in rows:
            print(f"{row['result']:4}  {row['case']:<24} {row['elapsed_ms']:>10} ms  {row['peak_rss_kib']:>8} KiB")
        print(f"evidence: {evidence}")
        return 0 if all_passed and performance_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
