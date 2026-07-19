#include "netforge/protocol.hpp"
#include "netforge/unique_fd.hpp"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

namespace {

bool parse_size(std::string_view text, std::size_t& value, bool allow_zero = false) {
  std::uint64_t parsed = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), parsed);
  if (error != std::errc{} || end != text.data() + text.size() ||
      (!allow_zero && parsed == 0) || parsed > std::numeric_limits<std::size_t>::max()) {
    return false;
  }
  value = static_cast<std::size_t>(parsed);
  return true;
}

netforge::UniqueFd connect_to(std::string_view host, std::string_view service) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo* raw_results = nullptr;
  const std::string host_text(host);
  const std::string service_text(service);
  if (::getaddrinfo(host_text.c_str(), service_text.c_str(), &hints, &raw_results) != 0) {
    return {};
  }
  const std::unique_ptr<addrinfo, decltype(&::freeaddrinfo)> results(raw_results, ::freeaddrinfo);
  for (auto* address = results.get(); address != nullptr; address = address->ai_next) {
    netforge::UniqueFd candidate(
        ::socket(address->ai_family, address->ai_socktype, address->ai_protocol));
    if (!candidate) continue;
    const int descriptor_flags = ::fcntl(candidate.get(), F_GETFD);
    if (descriptor_flags < 0 ||
        ::fcntl(candidate.get(), F_SETFD, descriptor_flags | FD_CLOEXEC) != 0) {
      continue;
    }
    timeval timeout{10, 0};
    if (::setsockopt(candidate.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0 ||
        ::setsockopt(candidate.get(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
      continue;
    }
#if defined(SO_NOSIGPIPE)
    int no_sigpipe = 1;
    if (::setsockopt(candidate.get(), SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
                     sizeof(no_sigpipe)) != 0) {
      continue;
    }
#endif
    if (::connect(candidate.get(), address->ai_addr, address->ai_addrlen) == 0) {
      return candidate;
    }
  }
  return {};
}

bool ping(int fd) {
  if (netforge::send_frame(fd, "PING").status != netforge::IoStatus::ok) return false;
  const auto response = netforge::recv_frame(fd);
  return response.status == netforge::IoStatus::ok && response.payload == "PONG";
}

long long percentile(const std::vector<long long>& sorted_nanoseconds, std::size_t percent) {
  const auto numerator = percent * sorted_nanoseconds.size();
  const auto rank = (numerator + 99U) / 100U;
  return sorted_nanoseconds[std::max<std::size_t>(1, rank) - 1];
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3 || argc > 6) {
    std::cerr << "usage: netforge-bench HOST PORT [REQUESTS] [CONCURRENCY] [WARMUP_PER_CONNECTION]\n";
    return 2;
  }

  std::size_t port = 0;
  std::size_t request_count = 1000;
  std::size_t concurrency = 4;
  std::size_t warmup_per_connection = 10;
  if (!parse_size(argv[2], port) || port > std::numeric_limits<std::uint16_t>::max() ||
      (argc > 3 && !parse_size(argv[3], request_count)) ||
      (argc > 4 && !parse_size(argv[4], concurrency)) || concurrency > 256 ||
      request_count > 10'000'000 ||
      (argc > 5 && !parse_size(argv[5], warmup_per_connection, true)) ||
      warmup_per_connection > 1'000'000) {
    std::cerr << "invalid numeric argument\n";
    return 2;
  }

  std::atomic<std::size_t> failures{0};
  std::vector<std::vector<long long>> worker_samples(concurrency);
  std::vector<std::jthread> workers;
  workers.reserve(concurrency);
  const auto wall_start = std::chrono::steady_clock::now();
  for (std::size_t worker = 0; worker < concurrency; ++worker) {
    workers.emplace_back([&, worker] {
      auto connection = connect_to(argv[1], argv[2]);
      if (!connection) {
        failures.fetch_add(1, std::memory_order_relaxed);
        return;
      }
      for (std::size_t warmup = 0; warmup < warmup_per_connection; ++warmup) {
        if (!ping(connection.get())) {
          failures.fetch_add(1, std::memory_order_relaxed);
          return;
        }
      }
      auto& samples = worker_samples[worker];
      const auto worker_request_count =
          request_count / concurrency + (worker < request_count % concurrency ? 1U : 0U);
      samples.reserve(worker_request_count);
      for (std::size_t request = 0; request < worker_request_count; ++request) {
        const auto start = std::chrono::steady_clock::now();
        if (!ping(connection.get())) {
          failures.fetch_add(1, std::memory_order_relaxed);
          return;
        }
        const auto stop = std::chrono::steady_clock::now();
        samples.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count());
      }
    });
  }
  workers.clear();
  const auto wall_stop = std::chrono::steady_clock::now();

  std::vector<long long> samples;
  for (auto& worker : worker_samples) {
    samples.insert(samples.end(), worker.begin(), worker.end());
  }
  if (failures.load(std::memory_order_relaxed) != 0 || samples.size() != request_count) {
    std::cerr << "benchmark failed: completed=" << samples.size()
              << " requested=" << request_count
              << " connection_or_io_failures=" << failures.load(std::memory_order_relaxed) << '\n';
    return 1;
  }
  std::sort(samples.begin(), samples.end());
  const double wall_seconds = std::chrono::duration<double>(wall_stop - wall_start).count();
  const auto to_microseconds = [](long long nanoseconds) {
    return static_cast<double>(nanoseconds) / 1000.0;
  };
  std::cout << "warmup=" << warmup_per_connection * concurrency
            << " requests=" << samples.size() << " concurrency=" << concurrency
            << " wall=" << wall_seconds << "s"
            << " wall_rate=" << (wall_seconds > 0.0 ? samples.size() / wall_seconds : 0.0)
            << "req/s"
            << " p50_us=" << to_microseconds(percentile(samples, 50))
            << " p95_us=" << to_microseconds(percentile(samples, 95))
            << " p99_us=" << to_microseconds(percentile(samples, 99))
            << " max_us=" << to_microseconds(samples.back()) << '\n';
  std::cout << "Scope: loopback, blocking persistent sessions, one request outstanding per session; "
               "percentiles exclude connect and warmup time.\n";
  return 0;
}
