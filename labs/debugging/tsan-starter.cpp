#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

struct Metrics {
  std::uint64_t completed{0};
};

int main() {
  constexpr unsigned worker_count = 4;
  constexpr std::uint64_t increments = 250'000;
  Metrics metrics;
  std::vector<std::jthread> workers;
  workers.reserve(worker_count);
  for (unsigned worker = 0; worker < worker_count; ++worker) {
    workers.emplace_back([&] {
      for (std::uint64_t index = 0; index < increments; ++index) {
        ++metrics.completed;  // Intentionally races.
      }
    });
  }
  for (auto& worker : workers) worker.join();
  std::cout << "completed=" << metrics.completed << '\n';
  return metrics.completed == worker_count * increments ? 0 : 1;
}

