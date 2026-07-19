#pragma once

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace netforge {

class KvStore {
 public:
  void set(std::string key, std::string value);
  [[nodiscard]] std::optional<std::string> get(std::string_view key) const;
  bool erase(std::string_view key);
  [[nodiscard]] std::size_t size() const;

 private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<std::string, std::string> values_;
};

}  // namespace netforge
