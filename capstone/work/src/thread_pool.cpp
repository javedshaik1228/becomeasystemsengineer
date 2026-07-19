#include "netforge/thread_pool.hpp"

namespace netforge {

// TODO(day05): start joining workers, drain work on shutdown, and justify ordering.
ThreadPool::ThreadPool(std::size_t, std::size_t queue_capacity) : tasks_(queue_capacity) {}
ThreadPool::~ThreadPool() { shutdown(); }
bool ThreadPool::submit(std::function<void()>) { return false; }
void ThreadPool::shutdown() { tasks_.close(); }
std::uint64_t ThreadPool::submitted() const noexcept { return 0; }
std::uint64_t ThreadPool::completed() const noexcept { return 0; }
void ThreadPool::worker_loop() {}

}  // namespace netforge
