#pragma once

#include <unistd.h>

#include <utility>

namespace netforge {

class UniqueFd {
 public:
  UniqueFd() noexcept = default;
  explicit UniqueFd(int fd) noexcept : fd_(fd) {}

  ~UniqueFd() { reset(); }

  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;

  UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}

  UniqueFd& operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
      reset(other.release());
    }
    return *this;
  }

  [[nodiscard]] int get() const noexcept { return fd_; }
  [[nodiscard]] explicit operator bool() const noexcept { return fd_ >= 0; }

  [[nodiscard]] int release() noexcept { return std::exchange(fd_, -1); }

  void reset(int replacement = -1) noexcept {
    const int old = std::exchange(fd_, replacement);
    if (old >= 0) {
      // Do not retry close() after EINTR: on Linux the descriptor is already
      // released and may have been reused by another thread.
      (void)::close(old);
    }
  }

 private:
  int fd_{-1};
};

}  // namespace netforge
