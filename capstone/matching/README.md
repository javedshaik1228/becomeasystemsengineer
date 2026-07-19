# Guided order matching engine capstone

This is an independent, step-by-step modern C++20 build. You start from compiling scaffolding in [`work/`](work/) and implement the engine through ten cumulative proof gates. The completed implementation is not the default project.

Open the [interactive capstone map](index.html) and begin with [Step 1: Model the contract](steps/01-contract.html).

## Milestones

1. Model order and fill contracts.
2. Build ordered bid/ask price levels.
3. Enforce price-time priority and partial fills.
4. Add GTC, IOC, duplicate rejection, and cancellation.
5. Route symbols through a deterministic single-writer engine.
6. Generate deterministic mock data and replay it.
7. Build a bounded lock-free ingress ring.
8. Connect producers, queue, engine, and clean shutdown.
9. Isolate Redis/Kafka egress behind bounded adapters.
10. Benchmark, profile, sanitize, and defend the design.

## Start

```bash
cmake -S capstone/matching/work -B build/matching-work -DMATCHING_STEP=1
cmake --build build/matching-work -j
ctest --test-dir build/matching-work --output-on-failure
```

The first test is expected to fail. Implement only the current step, rerun its gate, then raise `MATCHING_STEP`. Save design notes, failures, benchmark output, and explanations in `work/evidence/`.

## Learning contract

- Every step has one bounded outcome and an explicit “not yet” list.
- Tests are cumulative: Step N must retain Steps 1 through N−1.
- Progress lives under a capstone-only browser key and does not affect course progress.
- The optional `solution-reference/` is available only through each lesson’s 20/40/60-minute stuck protocol.
- Broker integration and performance claims come after core correctness and deterministic replay.
