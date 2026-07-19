# Four interview simulations

Run these with screen sharing or a plain editor and terminal. Record audio if
comfortable. No AI, autocomplete answers, or reference implementation during a
timed round; actual employer policies vary, and some supplied roles explicitly
restrict AI use in assessments.

At least one Day 13/14 mock needs an independent scorer before readiness is
considered evidence-backed. Ask the scorer to use the fixed scoped profile in
SCORECARD without seeing your self-score first. Share only clean-room course
artifacts and redact personal, customer, employer, credential, and proprietary
details. If you decline external review, keep the result explicitly labeled
**self-assessed / provisional**.

## Mock A — C++ implementation and diagnosis (60 min, Day 7)

- 5 min: clarify the incremental frame decoder contract.
- 30 min: implement Day 1's decoder from a blank file.
- 15 min: interviewer injects one oversized-length and one lifetime defect.
- 10 min: explain API ownership, complexity, exception/error policy, and tests.

Use SCORECARD dimensions C++, resource safety, debugging, and communication.
When practicing solo, use the deterministic [Mock A fault fixture](fixtures/mock-a/README.md)
for the injected-defect segment; copy it into the Day 7 evidence directory before
editing so the original prompt remains repeatable.

## Mock B — Concurrency, sockets, and Unix (75 min, Day 10)

- 15 min oral: TCP stream, half-close, `EINTR`/`EAGAIN`, descriptor inheritance.
- 25 min coding: bounded blocking queue with close/drain semantics.
- 20 min diagnosis: server hangs during shutdown; request stacks and trace evidence.
- 15 min follow-ups: atomics vs mutex, slow clients, backpressure, IPC alternatives.

The interviewer should challenge one correct answer with “why?” twice.
When practicing solo, use the deterministic [Mock B shutdown-hang fixture](fixtures/mock-b/README.md)
for the diagnosis segment. Its timeout, syscall trace, and all-thread backtrace
replace an interviewer inventing a failure on the spot.

## Mock C — Network service design (45 min, Day 13)

Design a secure configuration distribution service for 50,000 branch devices.
Cover requirements, capacity, API/protocol, certificate lifecycle, rollout,
revocation, retries/idempotency, observability, HA, and staged failure recovery.
Explain where TCP/TLS ends and SD-WAN/IPSec/PKI concerns begin; do not pretend they
are interchangeable.

Score the scoped design profile in SCORECARD: concurrency, networking/IPC,
debug/performance, storage/reliability, system design, and communication.

## Mock D — Choose one specialist design (45 min, Day 14)

### NAS/storage

Design a highly available file metadata/cache service. Discuss client-visible
semantics, cache invalidation, failover, idempotency, leases/locking awareness,
durability, packet/log/trace diagnosis, and how NFS/SMB would constrain behavior.

### Low-latency gateway

Design a market-data ingest and distribution gateway for a stated feed and
subscriber count. Define ordering and gap behavior, framing, bounded queues,
allocation policy, p50/p95/p99 target and measurement method, overload, replay,
affinity/cache-locality hypotheses, and the profile or hardware counter that
would justify each optimization. Separate the measured C++/systems design from
trading-domain assumptions; do not claim exchange, derivatives, NUMA, or
production low-latency experience you have not actually demonstrated.

### Unix/payments authorization

Design a multi-process authorization gateway on Unix: external socket sessions,
a Unix-domain control channel, and a bounded shared-memory data path. Define
descriptor and shared-memory ownership, synchronization/publication, process
restart, ordering, duplicates, idempotency, replay, audit durability, overload,
static analysis, and functional verification. You may use an ISO 8583-inspired
field schema only as an explicitly simplified assumption; do not claim payment
network or ISO 8583 production experience you do not have.

### L2/L3 networking

Design the software boundary for a switch MAC-learning/VLAN feature. Separate
control plane, data plane, SDK/HAL/SAI, ASIC state, reconciliation, warm restart,
packet/debug visibility, and distributed failure handling.

After Mock D, score the same recording once as implementer and once as skeptical
reviewer. The lower evidence-backed score wins.
Use the same scoped design profile as Mock C and its explicit normalization rule.
For independent calibration, the scorer may watch live or use an uninterrupted
recording/transcript. Save their timestamped row evidence under an alias or broad
role label; do not store private contact information.
