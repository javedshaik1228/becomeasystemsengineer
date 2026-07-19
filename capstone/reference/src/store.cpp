#include "netforge/store.hpp"

#include <mutex>
#include <utility>

namespace netforge {

void KvStore::set(std::string key, std::string value) {
  std::unique_lock lock(mutex_);
  values_.insert_or_assign(std::move(key), std::move(value));
}

std::optional<std::string> KvStore::get(std::string_view key) const {
  std::shared_lock lock(mutex_);
  const auto found = values_.find(std::string(key));
  if (found == values_.end()) {
    return std::nullopt;
  }
  return found->second;
}

bool KvStore::erase(std::string_view key) {
  std::unique_lock lock(mutex_);
  return values_.erase(std::string(key)) != 0;
}

std::size_t KvStore::size() const {
  std::shared_lock lock(mutex_);
  return values_.size();
}

}  // namespace netforge
