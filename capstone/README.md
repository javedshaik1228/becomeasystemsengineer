# NetForge capstone

NetForge is a deliberately small C++20 systems service with enough surface area
to expose real interview trade-offs: ownership, framing, short I/O, concurrency,
bounded work, persistence, recovery, debugging, and protocol hardening.

## Two tracks

- `work/` is yours. It compiles but starts with day-labelled incomplete behavior.
- `reference/` is the course author's complete baseline. Consult one matching
  function only after using the stuck protocol in COURSE.md.

Both tracks compile against the same tests:

```bash
cmake -S capstone -B build/reference -G Ninja -DNETFORGE_TRACK=reference
cmake --build build/reference
ctest --test-dir build/reference --output-on-failure

cmake -S capstone -B build/work -G Ninja -DNETFORGE_TRACK=work
cmake --build build/work
ctest --test-dir build/work -R day01 --output-on-failure
```

The first reference run should be green. The first work run is intentionally red.

## Daily code map

| Day | Primary files | Gate |
|---:|---|---|
| 1 | `src/protocol.cpp` framing and exact I/O | `day01` |
| 2 | `include/netforge/unique_fd.hpp` | `day02` |
| 3 | `src/protocol.cpp` typed command parser | `day03` |
| 4 | `blocking_queue.hpp`, `store.cpp` | `day04` |
| 5 | `thread_pool.cpp` | `day05` |
| 6 | `packet.cpp` | `day06` |
| 7 | `client.cpp`, `server.cpp` | `day07` |
| 8 | persistent framed session, server limits/readiness experiment | `day08` + design evidence |
| 9 | socketpair/Unix-domain control experiment | `day09` + ownership map |
| 10 | all failure paths | `day10` + tool evidence |
| 11 | `wal.cpp`, recovery behavior | `day11` |
| 12 | parser/packet/server boundary checks | `day12` |
| 13 | `work/ARCHITECTURE.md` | rubric, no code test |
| 14 | whole tree | `day14` + full matrix |

## Sanitizer matrix

Run ASan/UBSan and TSan in different build directories:

```bash
cmake -S capstone -B build/work-asan -G Ninja \
  -DNETFORGE_TRACK=work -DNETFORGE_SANITIZE_ADDRESS=ON
cmake --build build/work-asan
ctest --test-dir build/work-asan --output-on-failure

cmake -S capstone -B build/work-tsan -G Ninja \
  -DNETFORGE_TRACK=work -DNETFORGE_SANITIZE_THREAD=ON
cmake --build build/work-tsan
ctest --test-dir build/work-tsan --output-on-failure
```

## Static analysis

Run the same explicit clang-tidy profile from Linux/WSL or macOS and preserve its
output with the Day 10 and Day 14 evidence. Analyzer findings fail the command;
other bug-prone, performance, and portability diagnostics require a written
fix-or-disposition decision.

```bash
./scripts/analyze.sh work
./scripts/analyze.sh reference
```

## What the reference intentionally does not claim

It is a teaching baseline, not a production database. It handles sequential
framed requests on a persistent connection, but each active connection still
occupies one blocking worker. It uses a bounded worker pool, synchronously
`fsync`s each mutation, and uses a portable `poll` accept loop. It does not
implement pipelining, TLS, multi-record
transactions, WAL checksums/compaction, replication, authentication, or a fully
non-blocking per-connection state machine. Day 13 asks you to defend when each
would become necessary.

Read [PROTOCOL.md](PROTOCOL.md) before changing the wire contract.
