#pragma once

#include "netforge/blocking_queue.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace netforge {

class ThreadPool {
 public:
  ThreadPool(std::size_t workers, std::size_t queue_capacity);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  bool submit(std::function<void()> task);
  void shutdown();

  [[nodiscard]] std::uint64_t submitted() const noexcept;
  [[nodiscard]] std::uint64_t completed() const noexcept;

 private:
  void worker_loop();

  BlockingQueue<std::function<void()>> tasks_;
  std::vector<std::jthread> workers_;
  std::mutex join_mutex_;
  std::atomic<bool> shutting_down_{false};
  std::atomic<std::uint64_t> submitted_{0};
  std::atomic<std::uint64_t> completed_{0};
};

}  // namespace netforge
