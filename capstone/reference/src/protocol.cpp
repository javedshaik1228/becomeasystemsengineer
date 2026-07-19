#include "netforge/protocol.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netforge {
namespace {

std::string_view trim(std::string_view text) {
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
    text.remove_prefix(1);
  }
  while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
    text.remove_suffix(1);
  }
  return text;
}

std::pair<std::string_view, std::string_view> split_once(std::string_view text) {
  text = trim(text);
  const auto position = text.find_first_of(" \t\r\n");
  if (position == std::string_view::npos) {
    return {text, {}};
  }
  return {text.substr(0, position), trim(text.substr(position + 1))};
}

}  // namespace

std::vector<std::byte> encode_frame(std::string_view payload) {
  if (payload.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("frame payload exceeds the 32-bit wire length");
  }
  std::vector<std::byte> frame(sizeof(std::uint32_t) + payload.size());
  const auto network_length = htonl(static_cast<std::uint32_t>(payload.size()));
  std::memcpy(frame.data(), &network_length, sizeof(network_length));
  if (!payload.empty()) {
    std::memcpy(frame.data() + sizeof(network_length), payload.data(), payload.size());
  }
  return frame;
}

IoResult read_exact(int fd, std::span<std::byte> output) {
  std::size_t offset = 0;
  while (offset < output.size()) {
    const auto count = ::read(fd, output.data() + offset, output.size() - offset);
    if (count > 0) {
      offset += static_cast<std::size_t>(count);
      continue;
    }
    if (count == 0) {
      return {IoStatus::eof, offset, 0};
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return {IoStatus::would_block, offset, errno};
    }
    return {IoStatus::error, offset, errno};
  }
  return {IoStatus::ok, offset, 0};
}

IoResult write_all(int fd, std::span<const std::byte> input) {
  std::size_t offset = 0;
  while (offset < input.size()) {
#if defined(MSG_NOSIGNAL)
    const auto count = ::send(fd, input.data() + offset, input.size() - offset, MSG_NOSIGNAL);
#else
    const auto count = ::write(fd, input.data() + offset, input.size() - offset);
#endif
    if (count > 0) {
      offset += static_cast<std::size_t>(count);
      continue;
    }
    if (count == 0) {
      return {IoStatus::error, offset, EIO};
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return {IoStatus::would_block, offset, errno};
    }
    return {IoStatus::error, offset, errno};
  }
  return {IoStatus::ok, offset, 0};
}

FrameResult recv_frame(int fd, std::size_t max_payload) {
  std::array<std::byte, sizeof(std::uint32_t)> header{};
  const auto header_result = read_exact(fd, header);
  if (header_result.status != IoStatus::ok) {
    return {header_result.status, {}, header_result.error_number};
  }

  std::uint32_t network_length = 0;
  std::memcpy(&network_length, header.data(), sizeof(network_length));
  const auto length = static_cast<std::size_t>(ntohl(network_length));
  if (length > max_payload) {
    return {IoStatus::too_large, {}, EMSGSIZE};
  }

  std::string payload(length, '\0');
  auto bytes = std::as_writable_bytes(std::span(payload.data(), payload.size()));
  const auto payload_result = read_exact(fd, bytes);
  if (payload_result.status != IoStatus::ok) {
    return {payload_result.status, {}, payload_result.error_number};
  }
  return {IoStatus::ok, std::move(payload), 0};
}

IoResult send_frame(int fd, std::string_view payload) {
  const auto frame = encode_frame(payload);
  return write_all(fd, frame);
}

bool valid_key(std::string_view key) {
  if (key.empty() || key.size() > 128) {
    return false;
  }
  return std::all_of(key.begin(), key.end(), [](unsigned char character) {
    return std::isalnum(character) != 0 || character == '_' || character == '-' ||
           character == '.' || character == ':';
  });
}

ParseResult parse_command(std::string_view payload) {
  if (payload.find('\0') != std::string_view::npos) {
    return ParseError{"NUL bytes are not valid command text"};
  }
  const auto [verb, arguments] = split_once(payload);
  if (verb == "PING" && arguments.empty()) {
    return Command{Ping{}};
  }
  if (verb == "STATS" && arguments.empty()) {
    return Command{Stats{}};
  }
  if (verb == "GET" || verb == "DEL") {
    const auto [key, extra] = split_once(arguments);
    if (!valid_key(key) || !extra.empty()) {
      return ParseError{"GET/DEL requires exactly one safe key"};
    }
    if (verb == "GET") {
      return Command{Get{std::string(key)}};
    }
    return Command{Del{std::string(key)}};
  }
  if (verb == "SET") {
    const auto [key, value] = split_once(arguments);
    if (!valid_key(key) || value.empty()) {
      return ParseError{"SET requires a safe key and a non-empty value"};
    }
    return Command{Set{std::string(key), std::string(value)}};
  }
  return ParseError{"unknown or malformed command"};
}

}  // namespace netforge
