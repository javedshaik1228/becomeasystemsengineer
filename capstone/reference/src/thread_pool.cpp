#include "netforge/thread_pool.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netforge {
namespace {

thread_local ThreadPool* active_pool = nullptr;

}  // namespace

ThreadPool::ThreadPool(std::size_t workers, std::size_t queue_capacity)
    : tasks_(queue_capacity) {
  if (workers == 0) {
    throw std::invalid_argument("ThreadPool requires at least one worker");
  }
  try {
    workers_.reserve(workers);
    for (std::size_t index = 0; index < workers; ++index) {
      workers_.emplace_back([this] { worker_loop(); });
    }
  } catch (...) {
    // A partially constructed pool has no destructor to close the queue. Wake
    // and join any workers that were started before propagating the failure.
    tasks_.close();
    workers_.clear();
    throw;
  }
}

ThreadPool::~ThreadPool() { shutdown(); }

bool ThreadPool::submit(std::function<void()> task) {
  if (!task || shutting_down_.load(std::memory_order_acquire)) {
    return false;
  }
  // Publish the count before the queue can wake a fast worker; otherwise a
  // concurrent STATS read can transiently observe completed > submitted.
  submitted_.fetch_add(1, std::memory_order_relaxed);
  try {
    if (tasks_.push(std::move(task))) {
      return true;
    }
  } catch (...) {
    submitted_.fetch_sub(1, std::memory_order_relaxed);
    throw;
  }
  submitted_.fetch_sub(1, std::memory_order_relaxed);
  return false;
}

void ThreadPool::shutdown() {
  bool expected = false;
  if (shutting_down_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
    tasks_.close();
  }
  if (active_pool == this) {
    // A worker cannot join itself. It initiates queue closure; an external
    // shutdown call or the owner destructor performs the joins.
    return;
  }
  // Every external caller waits behind the join owner, so shutdown never
  // returns early while another caller is still draining accepted work.
  std::scoped_lock lock(join_mutex_);
  workers_.clear();
}

std::uint64_t ThreadPool::submitted() const noexcept {
  return submitted_.load(std::memory_order_relaxed);
}

std::uint64_t ThreadPool::completed() const noexcept {
  return completed_.load(std::memory_order_relaxed);
}

void ThreadPool::worker_loop() {
  active_pool = this;
  while (auto task = tasks_.pop()) {
    try {
      (*task)();
    } catch (...) {
      // The pool owns worker lifetime, not application exception policy.
    }
    completed_.fetch_add(1, std::memory_order_relaxed);
  }
  active_pool = nullptr;
}

}  // namespace netforge
