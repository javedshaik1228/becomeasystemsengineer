#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

namespace netforge {

// TODO(day04): add bounded storage, mutex/CVs, predicate waits, and close/drain semantics.
template <typename T>
class BlockingQueue {
 public:
  explicit BlockingQueue(std::size_t capacity) : capacity_(capacity) {
    if (capacity == 0) throw std::invalid_argument("capacity must be positive");
  }
  BlockingQueue(const BlockingQueue&) = delete;
  BlockingQueue& operator=(const BlockingQueue&) = delete;

  bool push(T) { return false; }
  std::optional<T> pop() { return std::nullopt; }
  void close() { closed_ = true; }
  [[nodiscard]] std::size_t size() const { return 0; }
  [[nodiscard]] bool closed() const { return closed_; }

 private:
  std::size_t capacity_;
  bool closed_{false};
};

}  // namespace netforge
