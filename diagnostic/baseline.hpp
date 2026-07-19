#pragma once

#include <cstddef>
#include <span>

namespace netforge::diagnostic {

enum class HeaderStatus {
  incomplete,
  ready,
  too_large,
};

struct HeaderResult {
  HeaderStatus status{HeaderStatus::incomplete};
  std::size_t frame_bytes{0};
};

// Inspect the first four-byte-big-endian length-prefixed frame without
// allocating or reading beyond bytes. frame_bytes includes header + payload.
[[nodiscard]] HeaderResult inspect_first_frame(std::span<const std::byte> bytes,
                                               std::size_t max_payload);

}  // namespace netforge::diagnostic
