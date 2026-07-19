#include "netforge/store.hpp"

namespace netforge {

// TODO(day04): define the synchronization invariant and implement these operations.
void KvStore::set(std::string, std::string) {}
std::optional<std::string> KvStore::get(std::string_view) const { return std::nullopt; }
bool KvStore::erase(std::string_view) { return false; }
std::size_t KvStore::size() const { return 0; }

}  // namespace netforge
