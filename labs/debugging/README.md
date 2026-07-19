# Debugging lab: capture red, explain, repair, prove green

Day 10 requires two real sanitizer diagnosis-and-fix cycles. The fixtures are
small so the report cannot be hidden inside NetForge's larger test suite.

## 1. Seed your editable copies

```bash
cp labs/debugging/asan-starter.cpp labs/debugging/asan-answer.cpp
cp labs/debugging/tsan-starter.cpp labs/debugging/tsan-answer.cpp
mkdir -p capstone/work/evidence/day-10
```

## 2. Preserve the expected failures

```bash
python3 labs/debugging/run.py --case asan --expect failure \
  | tee capstone/work/evidence/day-10/asan-red.txt
python3 labs/debugging/run.py --case tsan --expect failure \
  | tee capstone/work/evidence/day-10/tsan-red.txt
```

For each first report, write: violated invariant, allocation or participating
thread stacks, why the code compiled, one minimal repair, and one regression
assertion. Do not begin with a patch.

## 3. Repair independently and prove clean

Fix ownership in `asan-answer.cpp` without extending a view past its storage.
Fix synchronization in `tsan-answer.cpp` without `volatile`, detached threads,
or suppressing the report. Then run:

```bash
python3 labs/debugging/run.py --case asan --expect clean \
  | tee capstone/work/evidence/day-10/asan-green.txt
python3 labs/debugging/run.py --case tsan --expect clean \
  | tee capstone/work/evidence/day-10/tsan-green.txt
```

The runner accepts an expected failure only when the matching sanitizer emits a
report and exits non-zero. A clean result requires exit zero and no sanitizer
signature. Save the edited sources and `sanitizer-diagnosis.md` with both
red-to-green chains. These fixture results complement, not replace, the separate
sanitizer builds of the NetForge work track.

