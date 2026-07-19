# Low-latency order matching engine

This is the advanced capstone for the systems-engineering track: a deterministic, price-time-priority limit order book in modern C++20.

## Build and run (Linux, macOS, or WSL)

```bash
cmake -S capstone/matching -B build/matching -DCMAKE_BUILD_TYPE=Release
cmake --build build/matching -j
ctest --test-dir build/matching --output-on-failure
build/matching/matching-generate --output orders.csv --count 100000
build/matching/matching-engine orders.csv fills.csv
build/matching/matching-bench --count 1000000
```

The generated CSV is deliberately simple so it can be replayed, diffed, and fed to the engine during an interview.

## Design contract

* Matching is price-time priority: the best price wins, then the lowest sequence number (FIFO).
* A fill executes at the resting maker price. GTC orders rest; IOC remainders are discarded.
* The matching core is single-consumer. Prepare symbols before the hot loop, then consume the lock-free bounded MPMC `LockFreeIngress<Order>` from one engine thread. This keeps mutation of each book deterministic and avoids a mutex on the matching path.
* `OrderBook` owns all resting orders and cancellation indexes. `Fill` is an immutable event suitable for downstream publication.
* The queue is a bounded sequence-number ring (Vyukov-style); full/empty are explicit back-pressure signals.

## Redis and Kafka edges

`RedisPublisher` implements the small RESP2 `PUBLISH` command over a TCP socket. Keep it off the matching thread (publish fills from an event consumer or sidecar). `KafkaProducer` uses librdkafka when CMake finds it (`-DMATCHING_WITH_RDKAFKA=ON`); dependency-free builds emit the same versioned JSON envelope for `kcat -P` or a Kafka sidecar. The core therefore remains buildable without broker SDKs.

Example envelope:

```json
{"topic":"fills.v1","brokers":"localhost:9092","taker_id":42,"maker_id":7,"price_ticks":10025,"quantity":3}
```

## Interview extensions

1. Add cancel/replace and reject reasons without violating FIFO.
2. Replace the unordered-map cancellation scan with intrusive iterators and measure p50/p99 latency.
3. Add a single-producer/single-consumer queue benchmark and compare it with the MPMC queue.
4. Add crash recovery by journaling accepted commands and replaying the CSV protocol.
5. Put Redis/Kafka publishing behind an asynchronous event fan-out and demonstrate that broker stalls do not affect matching latency.

Use ThreadSanitizer and AddressSanitizer builds before claiming production readiness:

```bash
cmake -S capstone/matching -B build/tsan -DMATCHING_SANITIZE_THREAD=ON
cmake --build build/tsan -j
```
