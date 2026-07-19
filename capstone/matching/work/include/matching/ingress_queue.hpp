#pragma once

#include <cstddef>
#include <optional>

namespace matching {

// Step 7 TODO: replace this compiling scaffold with a bounded lock-free ring.
template <typename T>
class LockFreeIngress {
 public:
  explicit LockFreeIngress(std::size_t capacity) : capacity_(capacity) {}
  [[nodiscard]] bool try_push(T) { return false; }
  [[nodiscard]] std::optional<T> try_pop() { return std::nullopt; }
  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

 private:
  std::size_t capacity_;
};

}  // namespace matching
