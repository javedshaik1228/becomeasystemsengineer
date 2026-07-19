# Day 10 sanitizer diagnosis fixture

These two disposable programs make the first sanitizer report reproducible. They
are not NetForge code and must not be “fixed” in the production work tree.

```bash
mkdir -p capstone/work/evidence/day-10/fixtures
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined \
  interview/fixtures/day10/asan_uaf.cpp -o build/day10-asan-uaf
build/day10-asan-uaf 2>&1 | tee capstone/work/evidence/day-10/fixtures/asan-red.txt

clang++ -std=c++20 -O1 -g -fsanitize=thread -pthread \
  interview/fixtures/day10/tsan_race.cpp -o build/day10-tsan-race
build/day10-tsan-race 2>&1 | tee capstone/work/evidence/day-10/fixtures/tsan-red.txt
```

Before reading a fix, write the first failing access, owning allocation/thread,
and one hypothesis. Copy each source into the evidence directory, repair it, and
rerun with the same command. Save the clean outputs as `asan-green.txt` and
`tsan-green.txt`, plus `diagnosis.md` explaining why the sanitizer report—not a
changed exit code—establishes the repair.

