#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

std::uint64_t mix(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

std::uint64_t sum_range(std::uint64_t begin, std::uint64_t end) noexcept {
  std::uint64_t total = 0;
  for (auto value = begin; value < end; ++value) total += mix(value);
  return total;
}

std::uint64_t parallel_sum(std::uint64_t item_count, unsigned thread_count) {
  if (thread_count == 0) throw std::invalid_argument{"thread_count must be positive"};
  if (item_count == 0) return 0;
  const auto workers_to_use = static_cast<unsigned>(
      std::min<std::uint64_t>(item_count, thread_count));
  std::vector<std::uint64_t> partials(workers_to_use, 0);
  std::vector<std::jthread> workers;
  workers.reserve(workers_to_use);
  for (unsigned worker = 0; worker < workers_to_use; ++worker) {
    const auto begin = item_count * worker / workers_to_use;
    const auto end = item_count * (worker + 1U) / workers_to_use;
    workers.emplace_back([&, worker, begin, end] {
      partials[worker] = sum_range(begin, end);
    });
  }
  for (auto& worker : workers) worker.join();
  std::uint64_t total = 0;
  for (const auto partial : partials) total += partial;
  return total;
}

struct AdjacentCounter {
  std::atomic<std::uint64_t> value{0};
};

struct alignas(64) PaddedCounter {
  std::atomic<std::uint64_t> value{0};
};

template <typename Counter>
double counter_trial(unsigned thread_count, std::uint64_t iterations) {
  auto counters = std::make_unique<Counter[]>(thread_count);
  const auto start = Clock::now();
  std::vector<std::jthread> workers;
  workers.reserve(thread_count);
  for (unsigned worker = 0; worker < thread_count; ++worker) {
    workers.emplace_back([&, worker] {
      for (std::uint64_t index = 0; index < iterations; ++index) {
        counters[worker].value.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  for (auto& worker : workers) worker.join();
  const auto stop = Clock::now();
  for (unsigned worker = 0; worker < thread_count; ++worker) {
    if (counters[worker].value.load(std::memory_order_relaxed) != iterations) {
      throw std::runtime_error{"counter correctness failure"};
    }
  }
  return std::chrono::duration<double, std::milli>(stop - start).count();
}

double amdahl_ceiling(double serial_fraction, unsigned threads) {
  return 1.0 / (serial_fraction + (1.0 - serial_fraction) / threads);
}

}  // namespace

int main(int argc, char** argv) {
  const std::uint64_t item_count = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 4'000'000ULL;
  const std::uint64_t counter_iterations = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 750'000ULL;
  const unsigned hardware = std::max(1U, std::thread::hardware_concurrency());
  const unsigned maximum = std::min(8U, hardware);

  const auto serial_start = Clock::now();
  const auto expected = sum_range(0, item_count);
  const auto serial_stop = Clock::now();
  const double serial_ms = std::chrono::duration<double, std::milli>(serial_stop - serial_start).count();

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "items=" << item_count << " hardware_threads=" << hardware << '\n';
  std::cout << "threads,serial_ms,parallel_ms,speedup,efficiency,amdahl_ceiling_10pct\n";
  for (unsigned threads = 1; threads <= maximum; threads *= 2) {
    const auto start = Clock::now();
    const auto actual = parallel_sum(item_count, threads);
    const auto stop = Clock::now();
    if (actual != expected) {
      std::cerr << "checksum mismatch at " << threads << " thread(s)\n";
      return 1;
    }
    const double parallel_ms = std::chrono::duration<double, std::milli>(stop - start).count();
    const double speedup = serial_ms / parallel_ms;
    std::cout << threads << ',' << serial_ms << ',' << parallel_ms << ',' << speedup << ','
              << speedup / threads << ',' << amdahl_ceiling(0.10, threads) << '\n';
    if (threads > maximum / 2) break;
  }

  const unsigned counter_threads = std::max(2U, maximum);
  const double adjacent_ms = counter_trial<AdjacentCounter>(counter_threads, counter_iterations);
  const double padded_ms = counter_trial<PaddedCounter>(counter_threads, counter_iterations);
  std::cout << "counter_threads=" << counter_threads << " adjacent_ms=" << adjacent_ms
            << " padded_ms=" << padded_ms << '\n';
  std::cout << "PASS: checksums and counters are correct; interpret timing only across repeated runs.\n";
}

