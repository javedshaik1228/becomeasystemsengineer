#include "matching/redis_edge.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

namespace matching {
namespace {
std::string resp_bulk(std::string_view value) { return "$" + std::to_string(value.size()) + "\r\n" + std::string(value) + "\r\n"; }
}

RedisPublisher::RedisPublisher(std::string host, std::uint16_t port, std::string channel)
    : host_(std::move(host)), port_(port), channel_(std::move(channel)) {}

EdgeResult RedisPublisher::publish(std::string_view payload) const {
  addrinfo hints{}; hints.ai_socktype = SOCK_STREAM; hints.ai_family = AF_UNSPEC;
  addrinfo* addresses = nullptr;
  const auto port = std::to_string(port_);
  if (::getaddrinfo(host_.c_str(), port.c_str(), &hints, &addresses) != 0) return {false, "redis getaddrinfo failed"};
  int socket_fd = -1;
  for (auto* address = addresses; address != nullptr; address = address->ai_next) {
    socket_fd = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (socket_fd >= 0 && ::connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0) break;
    if (socket_fd >= 0) { ::close(socket_fd); socket_fd = -1; }
  }
  ::freeaddrinfo(addresses);
  if (socket_fd < 0) return {false, "redis connection failed"};
  const auto command = "*3\r\n" + resp_bulk("PUBLISH") + resp_bulk(channel_) + resp_bulk(payload);
#if defined(MSG_NOSIGNAL)
  const auto sent = ::send(socket_fd, command.data(), command.size(), MSG_NOSIGNAL);
#else
  const auto sent = ::send(socket_fd, command.data(), command.size(), 0);
#endif
  char response[128]{};
  const auto received = ::recv(socket_fd, response, sizeof(response) - 1U, 0);
  ::close(socket_fd);
  if (sent != static_cast<ssize_t>(command.size()) || received <= 0) return {false, "redis publish failed"};
  return {response[0] == ':', response[0] == ':' ? "published" : "redis rejected publish"};
}

}  // namespace matching
