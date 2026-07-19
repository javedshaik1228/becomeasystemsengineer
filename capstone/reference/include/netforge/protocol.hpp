#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace netforge {

inline constexpr std::size_t kDefaultMaxPayload = 64U * 1024U;

enum class IoStatus {
  ok,
  eof,
  would_block,
  error,
  too_large,
  malformed,
};

struct IoResult {
  IoStatus status{IoStatus::ok};
  std::size_t bytes{0};
  int error_number{0};
};

struct FrameResult {
  IoStatus status{IoStatus::ok};
  std::string payload;
  int error_number{0};
};

struct Ping {};
struct Stats {};
struct Get { std::string key; };
struct Del { std::string key; };
struct Set { std::string key; std::string value; };
using Command = std::variant<Ping, Stats, Get, Del, Set>;

struct ParseError { std::string message; };
using ParseResult = std::variant<Command, ParseError>;

[[nodiscard]] std::vector<std::byte> encode_frame(std::string_view payload);
[[nodiscard]] IoResult read_exact(int fd, std::span<std::byte> output);
[[nodiscard]] IoResult write_all(int fd, std::span<const std::byte> input);
[[nodiscard]] FrameResult recv_frame(int fd, std::size_t max_payload = kDefaultMaxPayload);
[[nodiscard]] IoResult send_frame(int fd, std::string_view payload);
[[nodiscard]] ParseResult parse_command(std::string_view payload);
[[nodiscard]] bool valid_key(std::string_view key);

}  // namespace netforge
