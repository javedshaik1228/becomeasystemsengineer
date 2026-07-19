# Parallelism lab: measure before claiming speedup

This Day 5 Core lab makes task parallelism, Amdahl's law, and false sharing
observable. It is deliberately separate from NetForge's worker pool: the pool
teaches lifetime and shutdown; this harness asks whether independent CPU work
actually scales.

## Contract

Implement the two `TODO` regions in `answer.cpp`:

1. Partition `[0, item_count)` into at most `thread_count` non-overlapping ranges.
2. Run one range per joining thread, store one partial sum per thread, join, and
   combine the partials without a race.

The parallel checksum must equal the serial checksum for thread counts 1, 2,
and the host's reported concurrency. Do not share a loop index, detach a worker,
or use a global accumulator. The timing comparison is evidence, not a test:
small workloads and noisy hosts can legitimately show no speedup.

```bash
cp labs/parallelism/starter.cpp labs/parallelism/answer.cpp
python3 labs/parallelism/run.py --source labs/parallelism/answer.cpp \
  | tee capstone/work/evidence/day-05/parallelism.txt
```

The runner compiles with C++20, warnings-as-errors, optimization, and pthreads.
It runs a checksum gate and prints serial/parallel duration, speedup, efficiency,
the 10%-serial Amdahl ceiling, and adjacent-vs-padded counter timings.

## Explain after measuring

- Why does `N` threads not imply `N×` speedup?
- Which portion is serial, and how does Amdahl's law bound the ceiling?
- Why can adjacent independent atomics contend without a data race?
- Why is one faster padded run insufficient proof of false sharing?
- What changes between task parallelism here and request concurrency in NetForge?

Save `parallelism.md` beside the raw output. Record the exact machine/thread
count, median of three runs, one confounder, and one experiment that could
falsify your explanation. If stuck for 60 minutes, inspect `reference.cpp`,
close it, and reimplement from memory.

