#pragma once

#include "matching/order.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>

namespace matching {

// Bounded Vyukov-style MPMC queue. Producers never take a mutex; each slot's
// sequence number provides ownership transfer and acquire/release visibility.
template <typename T>
class LockFreeIngress {
  static_assert(std::is_nothrow_move_constructible_v<T>);

  struct Cell {
    std::atomic<std::size_t> sequence{0};
    alignas(64) std::optional<T> value;
  };

 public:
  explicit LockFreeIngress(std::size_t requested_capacity)
      : capacity_(round_power_of_two(requested_capacity)), mask_(capacity_ - 1U), cells_(std::make_unique<Cell[]>(capacity_)) {
    for (std::size_t index = 0; index < capacity_; ++index) cells_[index].sequence.store(index, std::memory_order_relaxed);
  }

  LockFreeIngress(const LockFreeIngress&) = delete;
  LockFreeIngress& operator=(const LockFreeIngress&) = delete;

  [[nodiscard]] bool try_push(T item) noexcept {
    std::size_t position = enqueue_.load(std::memory_order_relaxed);
    for (;;) {
      Cell& cell = cells_[position & mask_];
      const std::size_t sequence = cell.sequence.load(std::memory_order_acquire);
      const auto difference = static_cast<std::ptrdiff_t>(sequence) - static_cast<std::ptrdiff_t>(position);
      if (difference == 0) {
        if (enqueue_.compare_exchange_weak(position, position + 1U, std::memory_order_relaxed)) {
          cell.value.emplace(std::move(item));
          cell.sequence.store(position + 1U, std::memory_order_release);
          return true;
        }
      } else if (difference < 0) {
        return false;
      } else {
        position = enqueue_.load(std::memory_order_relaxed);
      }
    }
  }

  [[nodiscard]] std::optional<T> try_pop() noexcept {
    std::size_t position = dequeue_.load(std::memory_order_relaxed);
    for (;;) {
      Cell& cell = cells_[position & mask_];
      const std::size_t sequence = cell.sequence.load(std::memory_order_acquire);
      const auto difference = static_cast<std::ptrdiff_t>(sequence) - static_cast<std::ptrdiff_t>(position + 1U);
      if (difference == 0) {
        if (dequeue_.compare_exchange_weak(position, position + 1U, std::memory_order_relaxed)) {
          std::optional<T> result(std::move(cell.value));
          cell.value.reset();
          cell.sequence.store(position + capacity_, std::memory_order_release);
          return result;
        }
      } else if (difference < 0) {
        return std::nullopt;
      } else {
        position = dequeue_.load(std::memory_order_relaxed);
      }
    }
  }

  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

 private:
  static std::size_t round_power_of_two(std::size_t requested) {
    std::size_t value = 2U;
    while (value < requested) value <<= 1U;
    return value;
  }

  const std::size_t capacity_;
  const std::size_t mask_;
  std::unique_ptr<Cell[]> cells_;
  alignas(64) std::atomic<std::size_t> enqueue_{0};
  alignas(64) std::atomic<std::size_t> dequeue_{0};
};

}  // namespace matching

