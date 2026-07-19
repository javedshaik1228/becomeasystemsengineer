#include "netforge/server.hpp"

#include "netforge/protocol.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <sys/time.h>
#include <type_traits>
#include <utility>

namespace netforge {
namespace {

template <class... Visitors>
struct Overloaded : Visitors... {
  using Visitors::operator()...;
};

template <class... Visitors>
Overloaded(Visitors...) -> Overloaded<Visitors...>;

}  // namespace

Server::Server(ServerOptions options)
    : options_(std::move(options)),
      wal_(options_.wal_path),
      pool_(std::max<std::size_t>(1, options_.workers),
            std::max<std::size_t>(1, options_.queue_capacity)) {
  // WAL recovery and the bundled client intentionally share the protocol's
  // bounded frame size. Reject an inconsistent server configuration before it
  // can acknowledge a record that a later restart cannot replay.
  if (options_.max_payload > kDefaultMaxPayload) {
    error_ = "max_payload exceeds the supported protocol limit";
    return;
  }
  if (!options_.wal_path.empty() && !wal_.enabled()) {
    error_ = "could not open the configured write-ahead log";
    return;
  }
  if (!wal_.recover(store_)) {
    error_ = "could not recover the write-ahead log";
    return;
  }
  open_listener();
}

Server::~Server() {
  request_stop();
  pool_.shutdown();
}

bool Server::ready() const noexcept { return static_cast<bool>(listener_); }
std::uint16_t Server::port() const noexcept { return bound_port_; }
const std::string& Server::error() const noexcept { return error_; }

void Server::open_listener() {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  addrinfo* raw_results = nullptr;
  const auto service = std::to_string(options_.port);
  const char* node = options_.bind_address.empty() ? nullptr : options_.bind_address.c_str();
  const int lookup = ::getaddrinfo(node, service.c_str(), &hints, &raw_results);
  if (lookup != 0) {
    error_ = ::gai_strerror(lookup);
    return;
  }
  const std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> results(raw_results, ::freeaddrinfo);

  for (auto* address = results.get(); address != nullptr; address = address->ai_next) {
    UniqueFd candidate(::socket(address->ai_family, address->ai_socktype, address->ai_protocol));
    if (!candidate) continue;
    const int descriptor_flags = ::fcntl(candidate.get(), F_GETFD);
    if (descriptor_flags < 0 ||
        ::fcntl(candidate.get(), F_SETFD, descriptor_flags | FD_CLOEXEC) != 0) {
      continue;
    }
    int reuse = 1;
    ::setsockopt(candidate.get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (::bind(candidate.get(), address->ai_addr, address->ai_addrlen) != 0) continue;
    if (::listen(candidate.get(), 128) != 0) continue;
    listener_ = std::move(candidate);
    break;
  }

  if (!listener_) {
    error_ = std::strerror(errno);
    return;
  }

  sockaddr_storage bound{};
  socklen_t bound_size = sizeof(bound);
  if (::getsockname(listener_.get(), reinterpret_cast<sockaddr*>(&bound), &bound_size) != 0) {
    error_ = std::strerror(errno);
    listener_.reset();
    return;
  }
  if (bound.ss_family == AF_INET) {
    bound_port_ = ntohs(reinterpret_cast<sockaddr_in*>(&bound)->sin_port);
  } else if (bound.ss_family == AF_INET6) {
    bound_port_ = ntohs(reinterpret_cast<sockaddr_in6*>(&bound)->sin6_port);
  }
}

void Server::run() {
  if (!listener_) return;
  pollfd event{listener_.get(), POLLIN, 0};
  while (!stop_requested_.load(std::memory_order_acquire)) {
    event.revents = 0;
    const int ready_count = ::poll(&event, 1, 100);
    if (ready_count < 0) {
      if (errno == EINTR) continue;
      error_ = std::strerror(errno);
      break;
    }
    if (ready_count == 0) continue;
    if ((event.revents & POLLIN) == 0) {
      if ((event.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) break;
      continue;
    }

    const int client_fd = ::accept(listener_.get(), nullptr, nullptr);
    if (client_fd < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
      error_ = std::strerror(errno);
      continue;
    }
    UniqueFd accepted(client_fd);
    const int descriptor_flags = ::fcntl(accepted.get(), F_GETFD);
    if (descriptor_flags < 0 ||
        ::fcntl(accepted.get(), F_SETFD, descriptor_flags | FD_CLOEXEC) != 0) {
      continue;
    }
    timeval timeout{2, 0};
    if (::setsockopt(accepted.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0 ||
        ::setsockopt(accepted.get(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
      continue;
    }
#if defined(SO_NOSIGPIPE)
    int enabled = 1;
    if (::setsockopt(accepted.get(), SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0) {
      continue;
    }
#endif
    try {
      // std::function requires a copyable task. Share the ownership holder,
      // then transfer the descriptor exactly once when the task starts.
      auto client = std::make_shared<UniqueFd>(std::move(accepted));
      if (!pool_.submit([this, client] { handle_client(UniqueFd(client->release())); })) {
        (void)send_frame(client->get(), "ERR overloaded");
      }
    } catch (...) {
      error_ = "could not queue accepted client work";
      break;
    }
  }
  pool_.shutdown();
}

void Server::request_stop() noexcept {
  stop_requested_.store(true, std::memory_order_release);
}

void Server::handle_client(UniqueFd client) {
  while (!stop_requested_.load(std::memory_order_acquire)) {
    const auto frame = recv_frame(client.get(), options_.max_payload);
    if (frame.status != IoStatus::ok) {
      if (frame.status == IoStatus::too_large) {
        (void)send_frame(client.get(), "ERR frame too large");
      }
      return;
    }
    if (send_frame(client.get(), execute(frame.payload)).status != IoStatus::ok) return;
  }
}

std::string Server::execute(std::string_view request_text) {
  const auto parsed = parse_command(request_text);
  if (const auto* parse_error = std::get_if<ParseError>(&parsed)) {
    return "ERR " + parse_error->message;
  }
  const auto& command = std::get<Command>(parsed);
  return std::visit(
      Overloaded{
          [](const Ping&) { return std::string("PONG"); },
          [this](const Stats&) {
            std::ostringstream output;
            output << "keys=" << store_.size() << " submitted=" << pool_.submitted()
                   << " completed=" << pool_.completed();
            return output.str();
          },
          [this](const Get& get) {
            const auto value = store_.get(get.key);
            return value ? "VALUE " + *value : std::string("NOT_FOUND");
          },
          [this](const Del& del) {
            std::scoped_lock lock(mutation_mutex_);
            if (!wal_.append_del(del.key)) return std::string("ERR WAL failure");
            return store_.erase(del.key) ? std::string("DELETED") : std::string("NOT_FOUND");
          },
          [this](const Set& set) {
            std::scoped_lock lock(mutation_mutex_);
            if (!wal_.append_set(set.key, set.value)) return std::string("ERR WAL failure");
            store_.set(set.key, set.value);
            return std::string("OK");
          }},
      command);
}

}  // namespace netforge
