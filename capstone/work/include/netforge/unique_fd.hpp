#pragma once

#include <unistd.h>

#include <utility>

namespace netforge {

// TODO(day02): make ownership unique, movable, and automatically released.
class UniqueFd {
 public:
  UniqueFd() noexcept = default;
  explicit UniqueFd(int fd) noexcept : fd_(fd) {}
  ~UniqueFd() = default;

  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;
  UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) {}
  UniqueFd& operator=(UniqueFd&& other) noexcept {
    fd_ = other.fd_;
    return *this;
  }

  [[nodiscard]] int get() const noexcept { return fd_; }
  [[nodiscard]] explicit operator bool() const noexcept { return fd_ >= 0; }
  [[nodiscard]] int release() noexcept { return std::exchange(fd_, -1); }
  void reset(int replacement = -1) noexcept { fd_ = replacement; }

 private:
  int fd_{-1};
};

}  // namespace netforge
