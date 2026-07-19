#pragma once

#include "netforge/store.hpp"
#include "netforge/thread_pool.hpp"
#include "netforge/unique_fd.hpp"
#include "netforge/wal.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>

namespace netforge {

struct ServerOptions {
  std::string bind_address{"127.0.0.1"};
  std::uint16_t port{0};
  std::size_t workers{4};
  std::size_t queue_capacity{64};
  std::size_t max_payload{64U * 1024U};
  std::filesystem::path wal_path{};
};

class Server {
 public:
  explicit Server(ServerOptions options);
  ~Server();

  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  [[nodiscard]] bool ready() const noexcept;
  [[nodiscard]] std::uint16_t port() const noexcept;
  [[nodiscard]] const std::string& error() const noexcept;

  void run();
  void request_stop() noexcept;

 private:
  void open_listener();
  void handle_client(UniqueFd client);
  [[nodiscard]] std::string execute(std::string_view request);

  ServerOptions options_;
  UniqueFd listener_;
  std::uint16_t bound_port_{0};
  std::string error_;
  std::atomic<bool> stop_requested_{false};
  std::mutex mutation_mutex_;
  KvStore store_;
  Wal wal_;
  ThreadPool pool_;
};

}  // namespace netforge
