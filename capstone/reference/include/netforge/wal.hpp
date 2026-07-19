#pragma once

#include "netforge/store.hpp"
#include "netforge/unique_fd.hpp"

#include <filesystem>
#include <mutex>
#include <string_view>

namespace netforge {

class Wal {
 public:
  explicit Wal(std::filesystem::path path);

  Wal(const Wal&) = delete;
  Wal& operator=(const Wal&) = delete;

  [[nodiscard]] bool enabled() const;
  bool append_set(std::string_view key, std::string_view value);
  bool append_del(std::string_view key);
  bool recover(KvStore& store) const;

 private:
  bool append_record(std::string_view record);

  std::filesystem::path path_;
  UniqueFd fd_;
  mutable std::mutex append_mutex_;
};

}  // namespace netforge
