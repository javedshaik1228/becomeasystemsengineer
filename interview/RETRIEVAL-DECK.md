# NetForge 1/3/7-day retrieval deck

Use this before opening the current lesson's notes. Answer aloud or on paper,
then check the referenced page only after committing to an answer. A miss is not
a failure: mark it for the browser review queue and retrieve it again after the
next sleep cycle.

## Day 01

- Baseline: What does TCP preserve, and what boundary must the application add?
- Baseline: Name one owner and one borrower in a safe C++ API.

## Day 02

- **D-1 — framing:** What invariant lets an exact-read loop distinguish progress,
  EOF, interruption, and failure without losing bytes?
- Transfer: Which operations must a file-descriptor owner delete or define?

## Day 03

- **D-1 — descriptors:** Trace ownership through move construction, move
  assignment over an existing descriptor, `release`, and destruction.
- Transfer: When may a borrowed `span` or `string_view` escape a function?

## Day 04

- **D-1 — interfaces:** Where does NetForge borrow request text, and at what
  boundary must it create owned strings?
- **D-3 — framing:** Why must the length bound be checked before allocation?

## Day 05

- **D-1 — queues:** State the predicate for producer wait, consumer wait, and
  closed-and-drained completion.
- **D-3 — RAII:** Why is destruction order part of a concurrent ownership proof?

## Day 06

- **D-1 — workers:** Give the shutdown order and the happens-before edge that
  makes shared-state destruction safe.
- **D-3 — parsing:** Explain why storing a view into a temporary request is not
  rescued by `const`.

## Day 07

- **D-1 — packet bounds:** In what order do you validate Ethernet, VLAN, IPv4,
  and TCP offsets before interpreting fields?
- **D-3 — synchronization:** Why must a condition-variable wait use the same
  mutex and predicate as the state transition?

## Day 08

- **D-1 — sockets:** Walk resolve → socket → connect/bind → listen → accept and
  name the owner after every successful call.
- **D-3 — thread lifetime:** How does cancellation reach a worker blocked on a
  queue versus a task blocked in `recv`?
- **D-7 — framing:** Explain one send split across reads and two frames coalesced
  into one read.

## Day 09

- **D-1 — readiness:** Distinguish readiness from completion and state where an
  output buffer owns unwritten bytes.
- **D-3 — packet path:** Decode source/destination address and port without
  relying on host endianness or struct layout.
- **D-7 — descriptor RAII:** Why can retrying `close` after `EINTR` be unsafe?

## Day 10

- **D-1 — IPC:** After `fork`, which inherited ends must close before EOF can be
  observed, and what synchronizes shared memory?
- **D-3 — TCP:** Contrast EOF, half-close, timeout, `EAGAIN`, and a malformed
  application frame.
- **D-7 — owned results:** Why does the parsed command own keys and values?

## Day 11

- **D-1 — diagnosis:** State observation → hypothesis → measurement → conclusion
  for one saved failure without naming a fix first.
- **D-3 — backpressure:** What bounded resource saturates first, and what is the
  admission or shedding policy?
- **D-7 — bounded queue:** Explain close-and-drain semantics to a skeptical
  interviewer.

## Day 12

- **D-1 — durability:** What ordering must hold among WAL append, `fsync`, state
  mutation, acknowledgement, crash, and recovery?
- **D-3 — process boundary:** Compare pipe, Unix-domain socket, and shared memory
  by ownership, framing, synchronization, and cleanup.
- **D-7 — thread pool:** Why are submitted/completed counters not a correctness
  proof?

## Day 13

- **D-1 — hardening:** Name three attacker-controlled dimensions besides payload
  bytes and the bound applied to each.
- **D-3 — evidence:** Which tool distinguishes a syscall stall, data race, memory
  error, and CPU hot path?
- **D-7 — packet reasoning:** Walk Ethernet/VLAN/IPv4/TCP bounds and one L2/L3
  control-vs-data-plane failure.

## Day 14

- **D-1 — design:** Start from users/SLOs/scale/failures and derive one queue
  capacity, overload policy, and observable signal.
- **D-3 — recovery:** Explain a torn WAL tail, why later appends cannot follow it,
  and what a restart must do.
- **D-7 — network service:** Defend framing, descriptor ownership, partial I/O,
  timeouts, shutdown, and the worker-versus-reactor trade-off as one causal path.

## Review rule

After each set, label every prompt **retrieved**, **partial**, or **missed**.
Re-answer partial/missed prompts the next day before adding new Stretch work.
After a correct answer, explain one changed assumption or counterexample; simple
recognition does not clear the item.
