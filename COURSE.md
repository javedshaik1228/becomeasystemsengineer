# NetForge course contract

This is a 14-day, approximately 40-hour sprint. The outcome is interview
readiness, not passive coverage. The published day pages are orchestration
pages; the machine-readable [curriculum manifest](curriculum-manifest.json)
breaks every Core day into 15–45 minute blocks with one action and one evidence
target. Complete the blocks in order and keep Stretch work outside the counted
budget. Each day follows the same loop:

1. **Recall (10 min):** answer yesterday's prompts without notes.
2. **Model (30–40 min):** study one small concept set from the lesson and primary source.
3. **Build (75–110 min):** make one vertical improvement to NetForge.
4. **Break and diagnose (25–40 min):** use tests, traces, sanitizers, or a packet capture.
5. **Explain (20–30 min):** answer the day's interview prompts aloud.
6. **Record (5 min):** save the required evidence and mark the lesson complete.

## Fourteen-day arc

| Day | Focus | Project increment | Proof gate | Time |
|---:|---|---|---|---:|
| 1 | Object lifetime, bytes, build loop | Safe length-prefixed frame codec | Day 1 tests + lifetime explanation | 2.5 h |
| 2 | RAII and resource ownership | Move-only `UniqueFd` and syscall errors | Leak-free descriptor tests | 2.5 h |
| 3 | Modern C++20 interfaces | Typed command parser using views/variants | Parser tests + API defense | 2.5 h |
| 4 | Memory model and synchronization | Bounded blocking queue | Race-free queue stress test | 3 h |
| 5 | Threads, cancellation, atomics, task parallelism | Joining thread pool plus scaling/false-sharing lab | Shutdown/load tests + checksum, speedup, efficiency, and Amdahl analysis | 3 h |
| 6 | Ethernet, IP, TCP, byte order | Packet-header decoder | Golden packet tests + packet walkthrough | 2.5 h |
| 7 | Blocking sockets and stream framing | TCP client/server vertical slice | Loopback integration test + `tcpdump` | 3 h |
| 8 | Non-blocking I/O and backpressure | Readiness-loop experiment | Slow-client test + design comparison | 3 h |
| 9 | IPC and process boundaries | Unix-domain control channel/shared memory lab | IPC round trip + ownership map | 2.5 h |
| 10 | Debugging and performance | Diagnose injected memory/race/latency bugs | Debugger/trace + ASan/TSan + clang-tidy + profiler hypothesis | 3 h |
| 11 | Filesystems, durability, storage | Write-ahead log and restart recovery | Crash/recovery tests + I/O path explanation | 3 h |
| 12 | Protocol and security hardening | Limits, timeouts, fuzz corpus, threat model | Abuse tests + security boundary review | 3 h |
| 13 | Systems design and specialist branches | Architecture/RCA packet; choose one of five role branches | 45-minute design + one bounded branch artifact | 3 h |
| 14 | Integrated capstone and mock loop | Benchmark, polish, and defend NetForge | All tests + two scored mock rounds | 4 h |

## Five selectable specialist branches

Day 13 deepens exactly one branch. The artifact proves bounded preparation; the
boundary prevents two weeks of study from being presented as production mastery.

| Branch | Concrete Day 13 artifact | Honest boundary |
|---|---|---|
| Secure networking | `branch/secure-networking.md`: trust-boundary diagram plus certificate rotation/revocation failure table and observable signals | Explains TLS/PKI/IPSec placement; does not claim cryptographic or routing implementation mastery |
| NAS/storage | `branch/nas-storage.md`: local-vs-NAS acknowledgement timeline plus cache/failover trace plan | Reasons about NFS/SMB state and diagnostics; does not claim protocol or distributed-storage mastery |
| L2/L3 data plane | `branch/l2-l3-data-plane.md`: desired/observed MAC-VLAN-LAG state table plus warm-restart reconciliation pseudocode and counters | Explains control/data/SAI/HAL boundaries; does not claim ASIC SDK or routing-protocol mastery |
| Unix/payments | `branch/unix-payments.md`: socket/shared-memory ownership sequence plus duplicate, ordering, and replay table | Demonstrates IPC and idempotency reasoning; does not invent ISO 8583 or payments-domain experience |
| Low latency | `branch/low-latency/answer.cpp`, runner evidence, and `tradeoffs.md` from the [closed-book line-reversal screen](interview/quantiq-line-reversal/README.md) | Measures one file workload; does not promise production latency, trading expertise, or affinity/NUMA gains |

Every `branch.md` names the selected row, links its artifact, states the hardest
failure and observable signal, and lists what you still do not know. Branch work
is a depth sample layered on the common C++/Unix/networking core, not a substitute
for any daily proof gate.

## Core and Stretch lanes

Day 1's diagnostic chooses your default lane. Under 60%, take every Core card and
skip Stretch until its day's proof gate passes. From 60–79%, do Core plus one
Stretch item per day. At 80% or above, use the compressed concept notes and spend
the saved time on failure injection, performance, and specialist branches.

Stretch work never replaces a proof gate. Advanced vocabulary without a working,
diagnosable system is not readiness.

## The project contract

The wire protocol is a four-byte big-endian payload length followed by UTF-8
command bytes. Supported commands are `PING`, `SET key value`, `GET key`,
`DEL key`, and `STATS`. The final service must:

- reject oversized, truncated, malformed, and slow frames without unbounded work;
- close every descriptor exactly once and survive failures on every syscall path;
- avoid data races, detached threads, unbounded queues, and lock-held blocking I/O;
- support clean startup, cancellation, shutdown, and restart recovery;
- expose enough counters and logs to debug load and failure behavior;
- pass unit, stress, integration, abuse, and recovery tests under sanitizers;
- include an architecture note with explicit invariants and trade-offs.

## Readiness scorecard

Score the final two mock loops independently. A pass is **80/100 or higher**, no
zero in any section, and no unresolved memory-safety/data-race defect.

| Dimension | Points | Observable evidence |
|---|---:|---|
| C++ correctness and API design | 20 | Lifetime, value semantics, containers, errors, modern interfaces |
| Resource and memory safety | 15 | RAII, ownership clarity, sanitizer-clean failure paths |
| Concurrency | 15 | Happens-before reasoning, cancellation, boundedness, contention |
| Networking and IPC | 15 | TCP stream semantics, partial I/O, readiness, byte order, process boundaries |
| Debugging and performance | 10 | Repro-first diagnosis, tools, measurement, bottleneck reasoning |
| Storage and reliability | 10 | I/O path, durability, recovery, consistency, failover concepts |
| System design | 10 | Requirements, estimates, APIs, failure modes, alternatives |
| Communication and ownership | 5 | Clear narration, questions, decisions, senior-level examples |

## Stuck protocol

After 20 focused minutes, write the failing observation and one hypothesis. After
40 minutes, inspect the relevant reference page. After 60 minutes, compare only
the smallest matching file in `capstone/reference/`, close it, and reimplement
from memory. Record what changed your model. This preserves desirable difficulty
without turning one defect into a lost evening.

## Learning records, only after evidence

Daily output belongs under `capstone/work/evidence/day-NN/`. A learning record is
rarer: create one only when evidence demonstrates non-trivial understanding,
corrects a misconception, establishes prior depth, or changes the mission.

```bash
./scripts/record-learning.sh descriptor-ownership \
  "Descriptor ownership became explicit" \
  "The failure-path test proved that the wrapper must be move-only; future socket work can assume one closing owner."
```

The helper lazily creates the next `learning-records/NNNN-slug.md` in the exact
Teach format. Mere coverage or a checked box does not qualify.

## What “done” means each day

A checked box is not evidence. Save the named command output or written artifact
under `capstone/work/evidence/day-NN/`, answer the retrieval prompts without
notes, and meet the automated or rubric gate. Browser progress is motivational;
the repository evidence is authoritative.
