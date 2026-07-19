#include "netforge/wal.hpp"

#include <utility>

namespace netforge {

// TODO(day11): append framed records, fsync before acknowledgement, and replay safely.
Wal::Wal(std::filesystem::path path) : path_(std::move(path)) {}
bool Wal::enabled() const { return path_.empty(); }
bool Wal::append_set(std::string_view, std::string_view) { return path_.empty(); }
bool Wal::append_del(std::string_view) { return path_.empty(); }
bool Wal::recover(KvStore&) const { return path_.empty(); }
bool Wal::append_record(std::string_view) { return path_.empty(); }

}  // namespace netforge
