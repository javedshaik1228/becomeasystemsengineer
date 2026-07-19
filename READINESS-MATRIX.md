# NetForge readiness evidence map

This map separates **course availability** from **interview readiness**. The
course is deployed; readiness is earned only by completing the named artifacts,
tests, explanations, and mocks. Browser progress is a convenience. Evidence in
`capstone/work/evidence/` is authoritative.

## Current state

- Curriculum and project assets: deployed for fourteen days / 2,430 minutes.
- Learner evidence: not yet established; the dashboard intentionally starts at
  0/14 and Day 1 chooses the Core or Stretch lane.
- Host prerequisite: WSL 2 is enabled on this Windows machine, but no Linux
  distribution or C++ toolchain is installed yet. See [SETUP.md](SETUP.md).
- Goal status: active until the Day 14 threshold is met. Having the materials is
  not evidence that the skills have been demonstrated.

## Requirement-to-proof matrix

| Required signal | Where it is learned and practiced | Evidence that proves it | State before study |
|---|---|---|---|
| Core C++ mechanics and memory | Days 1–3; ownership reference; Days 1–3 coding drills | Framing/parser gates, lifetime explanation, safe LRU implementation | Deployed; learner proof pending |
| Resource management | Day 2; `UniqueFd`; failure-path exercises | Descriptor closes exactly once; move/release/reset cases; oral ownership defense | Deployed; learner proof pending |
| Modern C++ standards | C++20 throughout; Days 2–5 emphasize Rule of Zero/Five, `span`, `variant`, `optional`, `jthread`, atomics | Compiling C++20 project plus API-design explanations without dangling views or confused ownership | Deployed; learner proof pending |
| Concurrency and parallelism | Days 4–5; Day 8 backpressure; concurrency reference; executable parallelism lab | Queue/pool gates, TSan-clean run, shutdown trace, happens-before explanation, correct parallel checksum, measured speedup/efficiency, Amdahl ceiling, and false-sharing hypothesis | Deployed; learner proof pending |
| Low-level networking | Day 6 packet decoder; networking reference; longest-prefix drill | Bounds-safe Ethernet/VLAN/IPv4/TCP decode, packet walkthrough, capture evidence | Deployed; learner proof pending |
| Socket programming | Days 1, 7–8; POSIX/socket reference | Loopback client/server, partial-I/O and timeout cases, slow-client experiment, socket oral | Deployed; learner proof pending |
| Unix IPC and process boundaries | Day 9; Mock B | Unix-domain/socketpair round trip, ownership map, shared-memory synchronization design | Deployed; learner proof pending |
| Debugging and performance | Days 10 and 14; debugging reference | Repro→hypothesis→measurement note, debugger/trace/sanitizer artifact, repeatable baseline | Deployed; learner proof pending |
| Storage, durability, and NAS reasoning | Day 11; storage reference; Days 13–14 NAS branch | WAL/restart tests, torn-tail reasoning, `fsync` contract, NAS design score | Deployed; learner proof pending |
| Security boundaries | Day 12; Day 13 network/security branch; Mock C | Abuse corpus, threat model, limit/timeout defense, PKI/revocation failure walk | Deployed; learner proof pending |
| L2/L3 and data-plane reasoning | Day 6; Day 13 L2/L3 branch; Mock D; route-lookup drill | MAC/VLAN/control-vs-data-plane design with SAI/HAL/ASIC reconciliation and failure signals | Deployed; learner proof pending |
| Low-latency and payments reasoning | Days 5, 9–10, 13–14; TTL/timer drills; Mock D | Measured tail-latency hypothesis, IPC/ordering/duplicate defense, audit/replay design | Deployed; learner proof pending |
| Senior system design and communication | Days 13–14; story bank; scorecard; Mocks C/D | 45-minute recording, capacity math, failure matrix, alternatives, concise senior story | Deployed; learner proof pending |
| Project-based integration | Daily NetForge work tree and common test suite | Completed work track, full tests, separate sanitizers, benchmark, architecture note | Deployed; learner proof pending |
| Linux/WSL or macOS execution | Setup/doctor/bootstrap scripts; VS Code tasks | Clean environment doctor and green reference baseline on the chosen platform | Setup available; runtime proof pending |

## Target-role routing

The shared Core lane is mandatory. Choose the branch closest to an interview only
after the Core proof gate for that day passes.

| Target family | Core emphasis | Specialist rehearsal | Honest two-week boundary |
|---|---|---|---|
| Cisco secure networking | C++ ownership, TCP/IP, distributed failure, performance | Day 12 threat model; Day 13 network/security; Mock C | Explain IPSec/IKEv2, PKI, and SD-WAN boundaries; do not claim production cryptography mastery |
| NetApp systems/storage | Linux internals, memory/thread bugs, profiling, HA reasoning | Day 11 WAL/recovery; NAS Mock D | Orient to filesystems, RAID, replication, NFS/SMB; do not claim ONTAP or protocol-internals mastery |
| HPE NAS/file protocols | Cross-layer C++/Unix diagnosis, packets/logs/traces | Day 11; NAS branch; NAS Mock D | Demonstrate cache/failover/locking awareness, not full SMB/NFS implementation expertise |
| HPE L2/L3/data plane | Packet parsing, route lookup, distributed debugging | Day 13 L2/L3 branch; L2/L3 Mock D | Demonstrate software boundaries and reconciliation; production ASIC SDK experience remains job-specific |
| Mastercard Unix/payments | C++ on Unix, sockets, IPC/shared memory, verification | Day 9; Mock B; low-latency/payments Mock D | Use an ISO 8583-inspired model only as orientation; never invent payments-domain experience |
| Low-latency C++ | Memory/concurrency, packet paths, profiling, cache/affinity reasoning | Days 5, 8, 10, 14; low-latency Mock D | Quantify measured behavior; hardware-specific mastery needs real target hardware and workload |

## Milestone gates

1. **Environment gate, before Day 1 build:** `doctor.sh` passes and the complete
   reference track is green.
2. **Vertical-slice gate, Day 7:** NetForge frames and serves real loopback TCP;
   Mock A exposes lifetime/framing gaps.
3. **Diagnosis gate, Day 10:** Mock B plus sanitizer, stack, syscall, and packet
   evidence demonstrates cross-layer reasoning.
4. **Senior-design gate, Day 13:** one architecture packet and one 45-minute
   branch design survive rubric scoring and skeptical follow-ups.
5. **Readiness gate, Day 14:** all tests pass; ASan/UBSan and TSan pass in separate
   builds; three comparable benchmark runs exist; coding/oral and design mocks
   each score at least 80/100, with no zero or automatic no-pass condition.

## What the course cannot prove for you

Two weeks cannot manufacture years of production ownership, vendor SDK exposure,
or deep NFS/SMB, routing, cryptography, payments, and trading-domain experience.
The course prepares truthful explanations, transferable systems skill, and
reviewable project evidence. Tailor stories only to work you actually performed.

When a gate fails, do not mark the day complete. Preserve the failure, use the
lesson's 20/40/60-minute stuck protocol, and repeat the original gate after a
spaced break.
