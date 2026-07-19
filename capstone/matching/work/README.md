# Learner workspace

This is the capstone you build. It starts as compiling scaffolding with deliberately failing step gates; it is not a finished engine.

Start at [Step 1](../steps/01-contract.html). Work only in this directory until a lesson explicitly tells you otherwise.

```bash
cmake -S capstone/matching/work -B build/matching-work -DMATCHING_STEP=1
cmake --build build/matching-work -j
ctest --test-dir build/matching-work --output-on-failure
```

Increase `MATCHING_STEP` only when the current proof gate passes. Every later gate includes the earlier behavior.

The optional `solution-reference/` exists for the stuck protocol described in the lessons. Do not browse it before the prescribed checkpoint; copying it defeats the retrieval and design work.
