#include "netforge/server.hpp"

#include <algorithm>
#include <utility>

namespace netforge {

// TODO(day07): bind/listen/accept and hand bounded work to the pool.
// TODO(day08): retain bounded frame state across sequential requests on one
// blocking connection, while keeping finite timeouts and explicit overload.
Server::Server(ServerOptions options)
    : options_(std::move(options)),
      wal_(options_.wal_path),
      pool_(std::max<std::size_t>(1, options_.workers),
            std::max<std::size_t>(1, options_.queue_capacity)) {
  error_ = "not implemented";
}
Server::~Server() { request_stop(); }
bool Server::ready() const noexcept { return false; }
std::uint16_t Server::port() const noexcept { return 0; }
const std::string& Server::error() const noexcept { return error_; }
void Server::run() {}
void Server::request_stop() noexcept { stop_requested_.store(true); }
void Server::open_listener() {}
void Server::handle_client(UniqueFd) {}
std::string Server::execute(std::string_view) { return "ERR not implemented"; }

}  // namespace netforge
