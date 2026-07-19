#include "netforge/wal.hpp"

#include "netforge/protocol.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <span>
#include <system_error>
#include <utility>

namespace netforge {
namespace {

bool write_file_all(int fd, std::span<const std::byte> bytes) {
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const auto count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
    if (count > 0) {
      offset += static_cast<std::size_t>(count);
      continue;
    }
    if (count < 0 && errno == EINTR) continue;
    return false;
  }
  return true;
}

bool sync_file(int fd) {
  while (::fsync(fd) != 0) {
    if (errno != EINTR) return false;
  }
  return true;
}

bool repair_torn_tail(int fd, off_t complete_size) {
  while (::ftruncate(fd, complete_size) != 0) {
    if (errno != EINTR) return false;
  }
  return sync_file(fd);
}

}  // namespace

Wal::Wal(std::filesystem::path path) : path_(std::move(path)) {
  if (path_.empty()) return;
  std::error_code error;
  if (!path_.parent_path().empty()) {
    std::filesystem::create_directories(path_.parent_path(), error);
    if (error) return;
  }
  fd_.reset(::open(path_.c_str(), O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC, 0600));
}

bool Wal::enabled() const {
  std::scoped_lock lock(append_mutex_);
  return static_cast<bool>(fd_);
}

bool Wal::append_set(std::string_view key, std::string_view value) {
  return append_record("SET " + std::string(key) + " " + std::string(value));
}

bool Wal::append_del(std::string_view key) {
  return append_record("DEL " + std::string(key));
}

bool Wal::append_record(std::string_view record) {
  std::scoped_lock lock(append_mutex_);
  if (path_.empty()) return true;
  if (!fd_) return false;
  const auto frame = encode_frame(record);
  if (!write_file_all(fd_.get(), frame) || !sync_file(fd_.get())) {
    // A failed append may have left a torn tail. Stop accepting later writes
    // so recovery never skips an unacknowledged tail and then replays newer
    // acknowledged records beyond it.
    fd_.reset();
    return false;
  }
  return true;
}

bool Wal::recover(KvStore& store) const {
  std::scoped_lock lock(append_mutex_);
  if (path_.empty()) return true;
  UniqueFd input(::open(path_.c_str(), O_RDWR | O_CLOEXEC));
  if (!input) return errno == ENOENT;

  off_t complete_size = 0;
  while (true) {
    std::array<std::byte, sizeof(std::uint32_t)> header{};
    const auto header_result = read_exact(input.get(), header);
    if (header_result.status == IoStatus::eof) {
      if (header_result.bytes == 0) return true;
      return repair_torn_tail(input.get(), complete_size);
    }
    if (header_result.status != IoStatus::ok) return false;

    std::uint32_t network_length = 0;
    std::memcpy(&network_length, header.data(), sizeof(network_length));
    const auto length = static_cast<std::size_t>(ntohl(network_length));
    if (length > kDefaultMaxPayload) return false;

    std::string payload(length, '\0');
    const auto body_result =
        read_exact(input.get(), std::as_writable_bytes(std::span(payload.data(), payload.size())));
    if (body_result.status == IoStatus::eof) {
      return repair_torn_tail(input.get(), complete_size);
    }
    if (body_result.status != IoStatus::ok) return false;

    const auto parsed = parse_command(payload);
    if (const auto* command = std::get_if<Command>(&parsed)) {
      if (const auto* set = std::get_if<Set>(command)) {
        store.set(set->key, set->value);
      } else if (const auto* del = std::get_if<Del>(command)) {
        store.erase(del->key);
      } else {
        return false;
      }
    } else {
      return false;
    }
    const off_t current_size = ::lseek(input.get(), 0, SEEK_CUR);
    if (current_size < 0) return false;
    complete_size = current_size;
  }
}

}  // namespace netforge
