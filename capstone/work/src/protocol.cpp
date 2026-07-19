#include "netforge/protocol.hpp"

namespace netforge {

// TODO(day01): encode the four-byte big-endian length and payload.
std::vector<std::byte> encode_frame(std::string_view) { return {}; }
IoResult read_exact(int, std::span<std::byte>) { return {IoStatus::error, 0, 0}; }
IoResult write_all(int, std::span<const std::byte>) { return {IoStatus::error, 0, 0}; }
FrameResult recv_frame(int, std::size_t) { return {IoStatus::error, {}, 0}; }
IoResult send_frame(int, std::string_view) { return {IoStatus::error, 0, 0}; }

// TODO(day03): parse into the Command variant without dangling views.
ParseResult parse_command(std::string_view) { return ParseError{"not implemented"}; }
bool valid_key(std::string_view) { return false; }

}  // namespace netforge
