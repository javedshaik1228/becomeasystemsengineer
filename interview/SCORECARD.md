# Technical-round scorecard

Use this after Days 7, 13, and 14. The interviewer scores observed behavior, not
confidence or vocabulary. Record one quote or code/design observation per row.
Copy [SCORECARD-TEMPLATE.md](SCORECARD-TEMPLATE.md) into the day's evidence
directory when you run a round; it includes independent-calibration and privacy
fields without storing private scorer contact details.

| Dimension | Weight | 0–1: unsafe | 2: developing | 3: interview-ready | 4: senior signal |
|---|---:|---|---|---|---|
| C++ correctness/API | 20 | UB or confused lifetime | Works on happy path | Clear ownership, invariants, complexity, tests | Simplifies API and anticipates maintenance |
| Resource safety | 15 | Leaks/double release | RAII with gaps | Failure paths and destruction order are sound | Makes invalid ownership unrepresentable |
| Concurrency | 15 | Data race/deadlock ignored | Uses locks without model | States happens-before, predicates, shutdown | Bounds sharing; measures contention |
| Networking/IPC | 15 | Treats TCP as messages | Basic calls only | Partial I/O, EOF, errors, byte order, limits | Compares worker/reactor and overload behavior |
| Debug/performance | 10 | Guesses fixes | Knows tool names | Repro→hypothesis→measurement→conclusion | Narrows cross-layer failures quickly |
| Storage/reliability | 10 | Confuses write/durability | Basic file API | `fsync`, recovery, cache/failure semantics | Connects local promises to NAS/HA trade-offs |
| System design | 10 | Feature dump | Components without math/failures | Requirements, capacity, invariants, alternatives | Drives ambiguity and prioritizes risks |
| Communication/ownership | 5 | Silent or evasive | Narrates implementation | Clarifies, structures, admits limits | Teaches trade-offs; uses concise evidence |

## Full versus scoped scores

For a full composite, rate all eight rows and convert each 0–4 rating to its
proportional weight. The weights sum to 100.

A focused mock cannot honestly score behavior it never exposes. Use these fixed
profiles rather than assigning zero to omitted rows or silently treating an
80-point subset as a 100-point score:

| Round | Included dimensions | Included weight |
|---|---|---:|
| Mock A | C++ correctness/API, resource safety, debug/performance, communication/ownership | 50 |
| Mock B / Day 14 coding + oral | C++ correctness/API, resource safety, concurrency, networking/IPC, debug/performance, communication/ownership | 80 |
| Mock C or D design | Concurrency, networking/IPC, debug/performance, storage/reliability, system design, communication/ownership | 65 |

For a scoped round, first calculate earned points only in the included rows, then
normalize: `score = 100 × earned included points ÷ included weight`. For example,
a rating of 3 in every Mock B row scores 75, not 60 and not 80. Omitted rows are
**not observed**, not zero. A pass is 80/100 or higher, no zero in an included
row, and no unresolved memory-safety or data-race defect. In L2/L3 or
low-latency design, use the storage/reliability row for explicit persistence,
restart, consistency, and failure-semantics reasoning; do not award NAS trivia
that the prompt never asks for.

## Independent calibration

Two passing self-scores establish **provisional, self-assessed readiness**. To
claim the course's evidence-backed readiness threshold, one Day 13 or Day 14 mock
must also be watched live or from an uninterrupted recording and scored by a
peer, mentor, or practicing engineer. The independent scorer should not see the
learner's score until submitting their own timestamped rows. Their scoped score
must also reach 80, with no zero or automatic no-pass condition. If the two
scores differ by more than 10 points, use the lower score, debrief the largest
evidence disagreement, and repeat that round after targeted practice.

Share only the minimum evidence needed: a clean-room exercise, terminal output,
and design/code produced for this course. Redact names, tokens, hostnames, paths,
customer data, employer code, and proprietary architecture. Audio is optional;
a timestamped transcript or scorer notes are sufficient. Record the scorer's
role in broad terms (for example, “senior backend peer”), an alias if preferred,
the date, included dimensions, evidence timestamps, and final score. Never put a
scorer's private contact details in the repository.

## Automatic no-pass conditions

- Cannot explain who closes a descriptor or joins a thread.
- Assumes one `recv` returns one application message.
- Uses `volatile` as thread synchronization.
- Allocates from an untrusted length before validating the bound.
- Claims `write` means durable without naming the persistence contract.
- Optimizes before establishing a repeatable baseline.

## Debrief

Write only three items: strongest demonstrated signal; highest-risk gap with exact
evidence; next experiment that could change the score. Do not average away a
critical defect with trivia points.
