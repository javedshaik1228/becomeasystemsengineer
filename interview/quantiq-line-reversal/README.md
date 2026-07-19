# Closed-book file line-reversal screen

This is a role-specific, 60-minute C++ screen for the low-latency branch: 50
minutes to implement, up to five focused minutes to run and preserve the
harness evidence, and five minutes to defend the design. It is deliberately
separate from NetForge so you must make file-I/O, memory, and measurement
decisions from a blank implementation rather than reuse capstone code.

## Source note and boundary

The live [Quantiq Software Engineer, C++ posting](https://job-boards.greenhouse.io/quantiq/jobs/4302276009)
publicly asks applicants to write a C++ program that reads one file, reverses
every line into another file, minimizes memory footprint and runtime, and does
not use AI tools during the application, assessments, or interviews.

That public statement is the only employer-sourced part of this exercise. The
50/5/5 schedule, command-line interface, byte/newline contract, fixtures,
runner, and review prompts below are NetForge practice conventions. They are
not leaked or inferred Quantiq tests, scoring thresholds, or implementation
requirements. Re-read the current employer instructions before any real
application and follow those instructions instead of this repository.

## Closed-book rules

For the first attempt:

1. Disconnect AI assistants and close this README after reading the contract.
2. Do not inspect `run.py`, notes, search results, or a prior solution.
3. You may use your editor, compiler, debugger, shell, and your own scratchpad.
4. Copy `starter.cpp` to the evidence directory and start a visible 50-minute
   implementation timer. Compile as often as you like, but do not run the
   supplied test runner until that timer ends.
5. Stop editing at 50 minutes, record the commit or source hash, then spend up
   to five focused minutes running and preserving the deterministic harness.
   Tool wall time may vary; do not edit while it runs.
6. Use the final five minutes for the oral defense below. Any later code fix is
   a separately labeled retake.

There is intentionally no reference solution in this repository.

## Local practice contract

Build a C++20 program with this interface:

```text
reverse-lines INPUT_FILE OUTPUT_FILE
```

- Read and write in binary mode. Treat file contents as bytes, not Unicode code
  points.
- A line ends with LF (`\n`) or CRLF (`\r\n`). Reverse only the content bytes
  and preserve that line's exact terminator. A standalone CR byte is content.
- Preserve line order, empty lines, embedded NUL bytes, and the absence of a
  final terminator. Never add a newline.
- INPUT_FILE and OUTPUT_FILE must be different paths. Reject a same-path request
  without modifying the input.
- Return zero only after the complete output is successfully written. Missing
  input, open/read/write/flush failures, bad arguments, and same-path requests
  return nonzero and print a concise diagnostic to standard error.
- Lines are not assumed to be small. The deterministic suite includes a single
  16 MiB line. Passing that case is not proof of bounded memory: state your
  asymptotic peak-memory model and identify what grows with the longest line.

The contract permits several defensible designs. A one-pass, line-buffered
implementation is simple and fast but uses O(longest line) memory. A seekable
file design can reverse fixed-size chunks with bounded working memory, at the
cost of more complicated I/O and a different policy for non-seekable input.
Make the trade-off explicit; do not claim O(1) memory while retaining a whole
line.

## Run the screen

From the repository root on Linux, WSL, or macOS, use the Day 14 evidence path
shown below. If this is your selected Day 13 branch artifact, substitute
`capstone/work/evidence/day-13/branch/low-latency` for both evidence paths and
link it from `day-13/branch.md`.

```bash
mkdir -p capstone/work/evidence/day-14/quantiq-line-reversal
cp interview/quantiq-line-reversal/starter.cpp \
  capstone/work/evidence/day-14/quantiq-line-reversal/answer.cpp

# Start the 50-minute implementation timer here. Run this after it ends.
python3 interview/quantiq-line-reversal/run.py \
  capstone/work/evidence/day-14/quantiq-line-reversal/answer.cpp \
  capstone/work/evidence/day-14/quantiq-line-reversal
```

The runner uses the compiler named by `CXX`, falling back to `c++`. It creates
deterministic byte fixtures, compiles with C++20 and optimization, runs each
case in a fresh process with a timeout, checks byte-for-byte output, and records
wall time and peak resident set size. A passing implementation gets three
additional repetitions of the 16 MiB case.

The evidence directory receives:

- `compile.txt`: exact compiler command and diagnostics;
- `environment.txt`: platform, compiler, source hash, and runner command;
- `results.tsv`: functional result, elapsed time, and peak RSS for every case;
- `performance.tsv`: three large-line measurements when that case passes;
- `summary.md`: pass/fail totals and measurement caveats;
- `logs/`: candidate stdout/stderr for diagnosis.

Elapsed time and RSS vary by compiler, allocator, filesystem, cache state, and
machine. The runner intentionally sets no universal performance pass mark.
Compare repeated runs on the same environment and only attribute an improvement
to a controlled change. Peak RSS includes process/runtime overhead and is not a
direct measurement of the algorithm's working buffer.

## Oral defense after the run

Without reopening notes, answer during the final five minutes:

1. What is the exact newline invariant, including CRLF and a missing final LF?
2. What is peak memory as a function of file size, longest line, and chunk size?
3. Which syscalls dominate, and what measurement would falsify that claim?
4. How do partial writes, disk-full errors, and output replacement affect the
   success contract?
5. What changes for a pipe or another non-seekable input?

Record the answers and one next experiment in `tradeoffs.md`. A fast number
without a correct output, environment record, memory model, and failure policy
is not a pass.
