#include "netforge/server.hpp"

#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <system_error>
#include <thread>

namespace {
volatile std::sig_atomic_t stop_requested = 0;

extern "C" void stop_server(int) {
  stop_requested = 1;
}

bool parse_port(std::string_view text, std::uint16_t& port) {
  std::uint32_t value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size() ||
      value > std::numeric_limits<std::uint16_t>::max()) {
    return false;
  }
  port = static_cast<std::uint16_t>(value);
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  netforge::ServerOptions options;
  if (argc > 1 && !parse_port(argv[1], options.port)) {
    std::cerr << "invalid port: " << argv[1] << '\n';
    return 2;
  }
  if (argc > 2) options.wal_path = argv[2];

  netforge::Server server(options);
  if (!server.ready()) {
    std::cerr << "server setup failed: " << server.error() << '\n';
    return 1;
  }
  std::signal(SIGINT, stop_server);
  std::signal(SIGTERM, stop_server);
  std::cout << "NetForge listening on " << options.bind_address << ':' << server.port() << '\n';
  std::atomic<bool> server_finished{false};
  std::jthread runner([&server, &server_finished] {
    server.run();
    server_finished.store(true, std::memory_order_release);
  });
  while (stop_requested == 0 && !server_finished.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  server.request_stop();
  runner.join();
  if (!server.error().empty()) std::cerr << "server stopped: " << server.error() << '\n';
  return 0;
}
