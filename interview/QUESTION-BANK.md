# C++ systems interview question bank

Answer aloud before opening the signal column. A strong response starts with the
rule/invariant, gives a concrete failure, then names a diagnostic or trade-off.

## Core C++ and ownership

| Prompt | Strong answer signals |
|---|---|
| When does an object's lifetime begin and end? | Storage vs lifetime; construction; destruction; placement/new or union edge awareness; UB on access outside lifetime |
| What should own a socket descriptor? | Move-only RAII handle; one closer; non-owning integer/view boundaries; destruction order |
| Rule of Zero vs Rule of Five? | Prefer composed RAII; define five only for direct ownership; exception/noexcept move implications |
| `unique_ptr` vs `shared_ptr`? | Ownership semantics first; control block/cycles/contention; `weak_ptr`; avoid “shared just in case” |
| `string_view` and `span` hazards? | Non-owning lifetime; invalidation; boundary validation; do not store past owner lifetime |
| What invalidates vector iterators? | Reallocation and operation-specific rules; reserve is capacity not lifetime guarantee |
| Why is signed overflow dangerous? | UB enables optimizer assumptions; use checked/unsigned arithmetic deliberately |
| Exception safety for resource acquisition? | RAII; strong/basic/nothrow guarantees; construct then commit; errors at C/syscall boundaries |

## Concurrency and memory model

| Prompt | Strong answer signals |
|---|---|
| Define a data race. | Conflicting unsynchronized accesses, at least one write, different threads; UB; happens-before |
| Why loop around condition-variable wait? | Spurious wakes; predicate protects state; mutex and predicate define protocol |
| Atomics or mutex? | Compound invariant vs independent scalar; clarity first; ordering and contention measured |
| Explain acquire/release. | Release publishes prior writes; acquire reading that value observes them; not a global fence story |
| Why can shutdown hang? | Blocking I/O/tasks, lost wake, lock order, join under dependency; cancellation ownership |
| Deadlock prevention? | Lock order, scoped multi-lock, no unknown/blocking calls under lock, timeouts are diagnosis not proof |
| False sharing? | Independent hot atomics on one cache line; coherence traffic; measure; padding/layout trade-off |
| `jthread` advantage and limit? | Joining destructor, stop token; cancellation cooperative; blocking syscall still needs wake/timeout |

## Linux, sockets, and IPC

| Prompt | Strong answer signals |
|---|---|
| Why can `read`/`write` be short? | Streams/readiness/signals/buffers; loop with progress; distinguish EOF/error/would-block |
| `EINTR` vs `EAGAIN`? | Interrupted operation vs no current progress on nonblocking/timeout; context-specific retry |
| Why not retry `close` after `EINTR` on Linux? | Descriptor may already be released/reused; retry risks closing unrelated resource |
| `fork` and descriptors? | Duplicated descriptor table entries share open-file description/state; close unused ends |
| Pipe vs Unix socket vs shared memory? | Direction/framing, descriptor passing/credentials, copy/serialization, synchronization/ownership complexity |
| Blocking pool vs reactor? | Simplicity and bounded threads vs readiness state machines; backpressure and slow-client behavior |
| What does half-close mean? | `shutdown` one direction; EOF after peer FIN; remaining direction can continue |
| Graceful server shutdown? | Stop accepts, reject/drain policy, wake blocking waits, bound client I/O, join workers, flush durability |

## Networking and protocols

| Prompt | Strong answer signals |
|---|---|
| Why frame over TCP? | Reliable ordered byte stream has no application message boundaries |
| Walk SYN to application bytes. | ARP/ND and route awareness, three-way handshake, queues, accept, seq/ack, segmentation not app framing |
| Network byte order? | Big-endian wire convention; explicit fixed-width conversion; never cast unaligned untrusted structs |
| MTU vs MSS? | Link packet limit vs TCP payload segment negotiation; fragmentation/PMTUD consequences |
| VLAN/MAC learning? | Tagged L2 domains; switch learns source MAC→port, floods unknown destination; aging/moves/loops |
| Routing vs forwarding? | Control plane computes state; data plane performs per-packet lookup/action; FIB/RIB distinction |
| TLS vs IPSec? | TLS protects application transport session; IPSec protects IP traffic/policies; authentication/key lifecycle layers |
| Parser hardening? | Length/overflow checks before allocation/copy; explicit states; limits/timeouts; fuzz corpus; fail closed |

## Storage, debugging, and performance

| Prompt | Strong answer signals |
|---|---|
| Does successful `write` mean durable? | Page cache; `fsync` file; directory metadata for create/rename; hardware/filesystem contract caveats |
| WAL ordering? | Append record and sync before acknowledging/applying visible mutation; replay idempotency/checksum/torn tail |
| Page cache role? | Buffered I/O, read-ahead/writeback, memory pressure, cache coherence differs across local/NAS clients |
| NFS/SMB interview depth? | Stateful/stateless history, opens/leases/locking/caching/failover at concept level; never claim full semantics casually |
| First step on crash? | Preserve evidence; reproduce/symbolize; stacks/registers/core; form competing hypotheses |
| `strace` vs debugger vs profiler? | Syscall/time behavior vs program state/control vs statistical cost; select from hypothesis |
| ASan vs TSan? | Memory errors/UB vs data races; separate builds; tool limits and false confidence |
| Tail-latency regression? | Stable workload; p50/p95/p99; CPU/syscalls/context switches/allocations/contention; before/after |

## Senior design and ownership

| Prompt | Strong answer signals |
|---|---|
| Start an ambiguous design. | Clarify users/SLO/scale/consistency/failures/security/non-goals before components |
| Bound overload. | Admission control, finite queues, deadlines, backpressure, rejection, load shedding, metrics |
| Capacity estimate. | Explicit units/assumptions; request and byte rates; concurrency via latency; headroom and bottleneck |
| Review an incident. | Timeline, contributing mechanisms, detection/recovery, durable corrective controls; no blame theater |
| Disagree with an architect. | Shared goal, evidence/options, reversible experiment, decision record, commit after decision |
| Mentor through a bug. | Questions and model-building; preserve ownership; tight feedback; systemic documentation/test improvement |
