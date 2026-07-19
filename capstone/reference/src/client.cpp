#include "netforge/client.hpp"

#include "netforge/protocol.hpp"
#include "netforge/unique_fd.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <cerrno>
#include <chrono>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace netforge {
namespace {

constexpr auto kConnectTimeout = std::chrono::seconds(2);

std::string error_message(int error_number) {
  return std::error_code(error_number, std::generic_category()).message();
}

bool restore_blocking_mode(int fd, int original_flags, int result_error) {
  if (::fcntl(fd, F_SETFL, original_flags) != 0 && result_error == 0) {
    result_error = errno;
  }
  errno = result_error;
  return result_error == 0;
}

bool connect_with_timeout(int fd, const sockaddr* address, socklen_t address_length) {
  const int original_flags = ::fcntl(fd, F_GETFL);
  if (original_flags < 0 || ::fcntl(fd, F_SETFL, original_flags | O_NONBLOCK) != 0) {
    return false;
  }

  if (::connect(fd, address, address_length) == 0) {
    return restore_blocking_mode(fd, original_flags, 0);
  }

  int result_error = errno;
  if (result_error != EINPROGRESS && result_error != EWOULDBLOCK) {
    (void)restore_blocking_mode(fd, original_flags, result_error);
    return false;
  }

  const auto deadline = std::chrono::steady_clock::now() + kConnectTimeout;
  pollfd event{fd, POLLOUT, 0};
  while (true) {
    const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline -
                                                              std::chrono::steady_clock::now());
    if (remaining.count() <= 0) {
      result_error = ETIMEDOUT;
      break;
    }
    event.revents = 0;
    const int ready = ::poll(&event, 1, static_cast<int>(remaining.count()));
    if (ready < 0) {
      if (errno == EINTR) continue;
      result_error = errno;
      break;
    }
    if (ready == 0) {
      result_error = ETIMEDOUT;
      break;
    }

    int socket_error = 0;
    socklen_t error_size = sizeof(socket_error);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &error_size) != 0) {
      result_error = errno;
    } else {
      result_error = socket_error;
    }
    break;
  }
  return restore_blocking_mode(fd, original_flags, result_error);
}

bool configure_socket(int fd) {
  const int descriptor_flags = ::fcntl(fd, F_GETFD);
  if (descriptor_flags < 0 || ::fcntl(fd, F_SETFD, descriptor_flags | FD_CLOEXEC) != 0) {
    return false;
  }
  timeval timeout{2, 0};
  if (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0 ||
      ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
    return false;
  }
#if defined(SO_NOSIGPIPE)
  int enabled = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) != 0) {
    return false;
  }
#endif
  return true;
}

}  // namespace

ClientResult request(std::string_view host, std::uint16_t port, std::string_view command) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* raw_results = nullptr;
  const auto service = std::to_string(port);
  const auto host_text = std::string(host);
  const int lookup = ::getaddrinfo(host_text.c_str(), service.c_str(), &hints, &raw_results);
  if (lookup != 0) return {false, {}, ::gai_strerror(lookup)};
  const std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> results(raw_results, ::freeaddrinfo);

  UniqueFd socket;
  int last_error = ECONNREFUSED;
  for (auto* address = results.get(); address != nullptr; address = address->ai_next) {
    UniqueFd candidate(::socket(address->ai_family, address->ai_socktype, address->ai_protocol));
    if (!candidate) {
      last_error = errno;
      continue;
    }
    if (!configure_socket(candidate.get())) {
      last_error = errno;
      continue;
    }
    if (connect_with_timeout(candidate.get(), address->ai_addr, address->ai_addrlen)) {
      socket = std::move(candidate);
      break;
    }
    last_error = errno;
  }
  if (!socket) return {false, {}, error_message(last_error)};

  const auto sent = send_frame(socket.get(), command);
  if (sent.status != IoStatus::ok) return {false, {}, error_message(sent.error_number)};
  const auto received = recv_frame(socket.get());
  if (received.status != IoStatus::ok) {
    return {false, {}, received.error_number == 0 ? "connection closed" :
                                                 error_message(received.error_number)};
  }
  return {true, received.payload, {}};
}

}  // namespace netforge
