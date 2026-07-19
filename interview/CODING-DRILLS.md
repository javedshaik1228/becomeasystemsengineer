# Fourteen systems-aligned coding drills

Use a 25-minute timer: 3 minutes clarify, 3 minutes state invariant and complexity,
15 minutes code, 4 minutes test aloud. No AI or reference code during the timer.
Afterward, compare the approach notes, not syntax. Put solutions under
`capstone/work/evidence/day-NN/drill.cpp` so later mocks can revisit them.

## Role-specific screen — file line reversal

The low-latency job research includes one unusually concrete public application
task: reverse every line from one file into another while minimizing memory and
runtime, with AI prohibited during the application and interviews. Run the
self-contained [closed-book file line-reversal screen](quantiq-line-reversal/README.md)
as a 60-minute branch exercise. Its starter, deterministic byte tests, 16 MiB
line, LF/CRLF/final-newline policy, timeout runner, and wall-time/peak-RSS
evidence are local practice conventions—not purported employer hidden tests.

Do not inspect the runner or use AI during the first timed attempt. This screen
is separate from the fourteen 25-minute drills below; when low latency is your
selected Day 13 branch, its source, results, and written memory model become the
branch artifact.

## Day 01 — Incremental frame decoder

Given arbitrary byte chunks, return every complete 4-byte-big-endian-length frame
and retain an incomplete suffix. Reject lengths over `max_frame` before allocating.

- Cases: split inside header; split inside payload; three frames in one chunk;
  zero-length frame; oversized length; truncated final chunk.
- Invariant: retained bytes are exactly a prefix of the next frame.
- Target: O(total bytes) time, O(max frame + retained suffix) space.

<details><summary>Approach check</summary>Maintain a byte buffer and read cursor. Parse a length only with four available bytes; validate it before waiting for payload. Emit by move/copy, advance, then compact only after useful progress. Explain why repeatedly erasing from the front can become quadratic.</details>

## Day 02 — LRU cache with stable ownership

Implement `get` and `put` for fixed-capacity string keys/values using a list plus
hash index. Define behavior for capacity zero and updates.

- Cases: hit promotes; update promotes; eviction; repeated key; zero capacity.
- Invariant: each map entry points to exactly one list node; list order is recency.
- Target: O(1) expected per operation, O(capacity) space.

<details><summary>Approach check</summary>Use `std::list` for stable iterators and `unordered_map<Key, list::iterator>`. Prefer Rule of Zero. Explain which object owns nodes and why raw iterators are non-owning.</details>

## Day 03 — CIDR membership

Parse an IPv4 CIDR such as `192.0.2.0/24` and determine whether input addresses
belong. Do not rely on undefined shifts for `/0` or `/32`.

- Cases: `/0`; `/32`; first/last address; malformed octet/prefix; leading zeros.
- Invariant: both address and network are compared under the same prefix mask.
- Target: O(input length) time, O(1) space.

<details><summary>Approach check</summary>Parse four bounded decimal octets into a `uint32_t`. Validate prefix 0–32. Construct `/0` separately; otherwise left-shift an all-ones unsigned value. Network order and host integer representation must be explicit.</details>

## Day 04 — Sliding-window maximum

Return the maximum for each contiguous window of size `k` in a latency sample
array. Reject invalid `k` explicitly.

- Cases: `k=1`; `k=n`; duplicates; decreasing/increasing; negative values.
- Invariant: deque indices are in-window and values are monotonically decreasing.
- Target: O(n) time, O(k) space.

<details><summary>Approach check</summary>Expire front indices before reading the maximum; remove dominated values from the back before adding the new index. Every index enters and leaves once.</details>

## Day 05 — Timer heap

Design a scheduler data structure with `schedule(id, deadline)`, `cancel(id)`, and
`pop_due(now)`. Cancellations must not make every operation linear.

- Cases: equal deadlines; reschedule; cancel missing; stale heap entries.
- Invariant: the map names the latest generation; stale heap nodes are ignored.
- Target: O(log n) schedule, amortized O(log n) pop/cancel.

<details><summary>Approach check</summary>Use a min-heap plus ID→generation/deadline map and lazy deletion. Explain the memory-growth trade-off and when indexed heaps become worthwhile.</details>

## Day 06 — Longest-prefix route lookup

Given IPv4 routes `(network, prefix, next_hop)`, return the matching route with
the longest prefix for a destination.

- Cases: default route; overlapping `/8` `/16` `/24`; no route; duplicate prefix.
- Invariant: the chosen route matches and no longer matching prefix exists.
- Target: first implement O(routes), then discuss a bitwise trie.

<details><summary>Approach check</summary>Correctness before a trie: validate and mask every stored network; track the best prefix length. For the stretch trie, walk destination bits and retain the most recent terminal node.</details>

## Day 07 — Connection interval merge

Merge overlapping or touching connection-activity intervals and return total busy
time. Define whether intervals are closed, open, or half-open.

- Cases: empty; nested; touching; unsorted; duplicate; large endpoints.
- Invariant: output is sorted and every pair of adjacent intervals is disjoint.
- Target: O(n log n) time, O(n) output.

<details><summary>Approach check</summary>Sort by start then end. Merge against the last output interval. Use a wide type for accumulated duration and state the overflow boundary.</details>

## Day 08 — Circular byte buffer

Implement bounded `write(span)`, `peek(span)`, and `consume(n)` for an output
buffer that may wrap. Never overwrite unread bytes.

- Cases: exact fill; wrap; partial write; consume zero/all; two-segment peek.
- Invariant: `0 <= size <= capacity`; head names first readable byte.
- Target: O(bytes copied), O(capacity) space.

<details><summary>Approach check</summary>Track head and size; derive tail as `(head + size) % capacity`. Each copy is at most two contiguous segments. Avoid sentinel-slot ambiguity by storing size explicitly.</details>

## Day 09 — Top-K endpoints

From `(endpoint, request_count)` records, return the `k` largest counts with
deterministic tie-breaking by endpoint.

- Cases: `k=0`; `k>n`; ties; duplicate endpoints; negative input rejected.
- Invariant: heap contains the best k records seen under the final comparator.
- Target: O(n log k) time, O(k) space.

<details><summary>Approach check</summary>Aggregate duplicates first, then keep a min-heap whose top is the worst retained record. Comparator consistency is the hard part; test ties explicitly.</details>

## Day 10 — Deadlock cycle detector

Given directed wait-for edges `A -> B`, return one cycle if present.

- Cases: self-loop; disconnected components; diamond without cycle; long cycle.
- Invariant: gray nodes are exactly the current DFS ancestry.
- Target: O(V+E) time and space.

<details><summary>Approach check</summary>Use white/gray/black DFS plus parent links; an edge to gray closes a cycle. An iterative version avoids stack depth; discuss which you would choose in production tooling.</details>

## Day 11 — WAL compaction

Given ordered `SET key value` and `DEL key` records, output the smallest replay log
that produces the same final map while preserving deterministic key order.

- Cases: update chain; delete after set; recreate; unknown delete; empty log.
- Invariant: replaying the compacted log yields the same final key/value map.
- Target: O(n + m log m), where m is live keys.

<details><summary>Approach check</summary>Replay into a map, erase on delete, then emit one SET per live key in sorted order. Explain why this loses intermediate history and cannot replace online crash-safe compaction without a cutover protocol.</details>

## Day 12 — Safe variable-length integer

Decode an unsigned 64-bit base-128 varint from bytes. Return value, bytes consumed,
or precise truncated/overflow/malformed errors.

- Cases: zero; max value; continuation at end; >10 bytes; invalid tenth byte.
- Invariant: no shift occurs by 64 or more; rejected input never wraps silently.
- Target: O(min(n,10)) time, O(1) space.

<details><summary>Approach check</summary>Validate byte count and high bits before shifting. Use unsigned arithmetic, distinguish incomplete from impossible encodings, and keep the input cursor unchanged on failure if the API promises transactional parsing.</details>

## Day 13 — Failure-domain placement

Assign replicas to hosts across racks so each item has three replicas on distinct
racks when possible. Return a clear degraded result when constraints cannot hold.

- Cases: fewer than three racks; uneven racks; host exclusions; deterministic seed.
- Invariant: no duplicate host; rack diversity is maximal under available inputs.
- Target: state a reasonable greedy complexity and its limitation.

<details><summary>Approach check</summary>Group healthy hosts by rack, choose racks before hosts, and rotate deterministically for balance. This is a design/coding prompt: name why greedy placement is not globally optimal and what metrics a real allocator needs.</details>

## Day 14 — TTL cache, then defend it

Implement a cache with `put(key,value,ttl)`, `get(key,now)`, and `purge(now)`.
Target O(1) expected lookup and efficient expiry. Then explain the synchronization
strategy for multiple readers/writers.

- Cases: immediate expiry; overwrite; stale timer entry; clock tie; purge batch.
- Invariant: `get` never returns a value whose current generation is expired.
- Target: hash map + min-heap, O(log n) update/purge, O(1) expected lookup.

<details><summary>Approach check</summary>Combine a map containing value/expiry/generation with a min-heap and lazy stale-entry removal. In the concurrency defense, start with one mutex; partition only after naming a measured contention problem.</details>
