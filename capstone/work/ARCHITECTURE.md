# NetForge architecture — learner copy

Complete this on Day 13. Keep it to two pages plus one diagram.

## Requirements and non-goals

State the workload, correctness promise, latency/throughput target, failure model,
and explicit non-goals.

## Components and ownership

Name every long-lived object, owning thread, descriptor owner, and shared state.
Describe destruction order and graceful shutdown.

## Request path

Trace one `SET` from TCP bytes through framing, parsing, scheduling, WAL sync,
store mutation, and response. Mark every blocking boundary and allocation.

## Invariants

- TODO: protocol invariant
- TODO: concurrency invariant
- TODO: durability invariant
- TODO: overload invariant

## Capacity and overload

Estimate connections, queue memory, worker utilization, log growth, and the first
bottleneck. Define rejection and timeout behavior.

## Failures and observability

Cover malformed clients, slow clients, worker exhaustion, disk full, torn WAL,
process crash, restart, and partial network failure. Name the metric/log/trace
that distinguishes each hypothesis.

## Alternatives rejected

Compare at least: thread per connection, bounded worker pool, reactor; synchronous
WAL vs group commit; local persistence vs replicated storage.

## Specialist branch

Choose one: network/security, NAS/storage, or low-latency/payments. Explain three
ways its constraints change this design without claiming you implemented the
external protocol.
