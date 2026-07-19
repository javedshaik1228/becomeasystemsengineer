#include "netforge/blocking_queue.hpp"
#include "netforge/client.hpp"
#include "netforge/packet.hpp"
#include "netforge/protocol.hpp"
#include "netforge/server.hpp"
#include "netforge/store.hpp"
#include "netforge/thread_pool.hpp"
#include "netforge/unique_fd.hpp"
#include "netforge/wal.hpp"

#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using namespace std::chrono_literals;

struct Checks {
  int failures{0};

  void expect(bool condition, std::string_view message) {
    if (!condition) {
      ++failures;
      std::cerr << "  FAIL: " << message << '\n';
    }
  }
};

void close_now(netforge::UniqueFd& descriptor) {
  const int fd = descriptor.release();
  if (fd >= 0) (void)::close(fd);
}

std::filesystem::path temporary_path(std::string_view label) {
  auto pattern = (std::filesystem::temp_directory_path() /
                  ("netforge-" + std::string(label) + "-XXXXXX"))
                     .string();
  std::vector<char> writable(pattern.begin(), pattern.end());
  writable.push_back('\0');
  const int fd = ::mkstemp(writable.data());
  if (fd < 0) {
    throw std::system_error(errno, std::generic_category(), "mkstemp");
  }
  if (::close(fd) != 0) {
    const int close_error = errno;
    std::filesystem::remove(writable.data());
    throw std::system_error(close_error, std::generic_category(), "close temporary file");
  }
  if (!std::filesystem::remove(writable.data())) {
    throw std::runtime_error("could not prepare a unique temporary WAL path");
  }
  return writable.data();
}

int day01() {
  Checks checks;
  const auto encoded = netforge::encode_frame("PING");
  checks.expect(encoded.size() == 8, "frame is four-byte length plus payload");
  checks.expect(std::to_integer<unsigned>(encoded[3]) == 4, "length is big-endian");
  checks.expect(netforge::encode_frame("").size() == 4, "empty payload has a valid header");

  int pair[2]{};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
    checks.expect(false, "socketpair created");
    return checks.failures;
  }
  netforge::UniqueFd left(pair[0]);
  netforge::UniqueFd right(pair[1]);
  checks.expect(netforge::send_frame(left.get(), "hello").status == netforge::IoStatus::ok,
                "frame writes completely");
  const auto frame = netforge::recv_frame(right.get());
  checks.expect(frame.status == netforge::IoStatus::ok && frame.payload == "hello",
                "frame round-trips over a byte stream");
  close_now(left);
  close_now(right);

  int fragmented_pair[2]{};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fragmented_pair) != 0) {
    checks.expect(false, "fragmented socketpair created");
    return checks.failures;
  }
  netforge::UniqueFd fragmented_reader(fragmented_pair[0]);
  netforge::UniqueFd fragmented_writer(fragmented_pair[1]);
#if defined(SO_NOSIGPIPE)
  int fragmented_no_sigpipe = 1;
  (void)::setsockopt(fragmented_writer.get(), SOL_SOCKET, SO_NOSIGPIPE,
                     &fragmented_no_sigpipe, sizeof(fragmented_no_sigpipe));
#endif
  const auto fragmented_bytes = netforge::encode_frame("split-across-header-and-body");
  std::atomic<bool> writer_ok{true};
  std::jthread fragmenter([fd = fragmented_writer.release(), fragmented_bytes, &writer_ok] {
    netforge::UniqueFd endpoint(fd);
    for (const auto byte : fragmented_bytes) {
      while (true) {
#if defined(MSG_NOSIGNAL)
        const auto count = ::send(endpoint.get(), &byte, 1, MSG_NOSIGNAL);
#else
        const auto count = ::write(endpoint.get(), &byte, 1);
#endif
        if (count == 1) break;
        if (count < 0 && errno == EINTR) continue;
        writer_ok.store(false);
        close_now(endpoint);
        return;
      }
      std::this_thread::sleep_for(100us);
    }
    close_now(endpoint);
  });
  const auto fragmented_frame = netforge::recv_frame(fragmented_reader.get());
  checks.expect(writer_ok.load(), "fragment writer made progress");
  checks.expect(fragmented_frame.status == netforge::IoStatus::ok &&
                    fragmented_frame.payload == "split-across-header-and-body",
                "frame reader accumulates a header and body delivered one byte at a time");
  close_now(fragmented_reader);
  fragmenter.join();

  int truncated_pair[2]{};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, truncated_pair) != 0) {
    checks.expect(false, "truncated socketpair created");
    return checks.failures;
  }
  netforge::UniqueFd truncated_reader(truncated_pair[0]);
  netforge::UniqueFd truncated_writer(truncated_pair[1]);
  const auto complete = netforge::encode_frame("truncated");
  const auto prefix =
      std::span<const std::byte>(complete.data(), complete.size()).first(complete.size() - 2);
  checks.expect(netforge::write_all(truncated_writer.get(), prefix).status ==
                    netforge::IoStatus::ok,
                "truncated frame prefix is written");
  close_now(truncated_writer);
  const auto truncated_frame = netforge::recv_frame(truncated_reader.get());
  checks.expect(truncated_frame.status == netforge::IoStatus::eof,
                "EOF in a declared body is reported instead of accepted as a frame");
  close_now(truncated_reader);
  return checks.failures;
}

int day02() {
  Checks checks;
  static_assert(!std::is_copy_constructible_v<netforge::UniqueFd>);
  static_assert(std::is_nothrow_move_constructible_v<netforge::UniqueFd>);
  int descriptors[2]{};
  if (::pipe(descriptors) != 0) {
    checks.expect(false, "pipe created");
    return checks.failures;
  }
  const int observed = descriptors[0];
  {
    netforge::UniqueFd first(descriptors[0]);
    netforge::UniqueFd second(std::move(first));
    checks.expect(!first && second.get() == observed, "move transfers sole ownership");
  }
  errno = 0;
  const int descriptor_status = ::fcntl(observed, F_GETFD);
  checks.expect(descriptor_status == -1 && errno == EBADF,
                "destructor closes the descriptor exactly once");
  if (descriptor_status != -1 || errno != EBADF) (void)::close(observed);
  ::close(descriptors[1]);

  int first_pipe[2]{-1, -1};
  int second_pipe[2]{-1, -1};
  if (::pipe(first_pipe) != 0 || ::pipe(second_pipe) != 0) {
    checks.expect(false, "move-assignment pipes created");
    if (first_pipe[0] >= 0) (void)::close(first_pipe[0]);
    if (first_pipe[1] >= 0) (void)::close(first_pipe[1]);
    if (second_pipe[0] >= 0) (void)::close(second_pipe[0]);
    if (second_pipe[1] >= 0) (void)::close(second_pipe[1]);
    return checks.failures;
  }
  const int replaced = first_pipe[0];
  const int transferred = second_pipe[0];
  netforge::UniqueFd destination(replaced);
  netforge::UniqueFd source(transferred);
  destination = std::move(source);
  errno = 0;
  checks.expect(::fcntl(replaced, F_GETFD) == -1 && errno == EBADF,
                "move assignment releases the destination's previous descriptor");
  checks.expect(!source && destination.get() == transferred,
                "move assignment transfers sole ownership");
  destination = std::move(destination);
  checks.expect(destination.get() == transferred,
                "self move assignment preserves the ownership invariant");
  const int released = destination.release();
  checks.expect(!destination && released == transferred,
                "release returns the descriptor and clears ownership");
  (void)::close(released);
  (void)::close(first_pipe[1]);
  (void)::close(second_pipe[1]);
  return checks.failures;
}

int day03() {
  Checks checks;
  auto parsed = netforge::parse_command("SET session:42 value with spaces");
  const auto* command = std::get_if<netforge::Command>(&parsed);
  checks.expect(command != nullptr && std::holds_alternative<netforge::Set>(*command),
                "SET becomes a typed command");
  if (command != nullptr) {
    if (const auto* set = std::get_if<netforge::Set>(command)) {
      checks.expect(set->key == "session:42" && set->value == "value with spaces",
                    "views are copied only at the ownership boundary");
    }
  }
  checks.expect(std::holds_alternative<netforge::ParseError>(netforge::parse_command("GET a b")),
                "extra arguments are rejected");
  checks.expect(std::holds_alternative<netforge::ParseError>(netforge::parse_command("SET ../x y")),
                "unsafe keys are rejected");
  checks.expect(std::holds_alternative<netforge::ParseError>(
                    netforge::parse_command(std::string("GET a\0hidden", 12))),
                "embedded NUL bytes are rejected");
  checks.expect(std::holds_alternative<netforge::ParseError>(
                    netforge::parse_command("SET key")),
                "SET without a value is rejected");
  checks.expect(std::holds_alternative<netforge::ParseError>(
                    netforge::parse_command("PING extra")),
                "zero-argument commands reject trailing input");
  const auto owned_from_temporary = netforge::parse_command(std::string("SET temp durable"));
  const auto* owned_command = std::get_if<netforge::Command>(&owned_from_temporary);
  const auto* owned_set = owned_command == nullptr ? nullptr : std::get_if<netforge::Set>(owned_command);
  checks.expect(owned_set != nullptr && owned_set->key == "temp" && owned_set->value == "durable",
                "returned commands own data that outlives temporary parser input");
  return checks.failures;
}

int day04() {
  Checks checks;
  netforge::BlockingQueue<int> queue(2);
  std::vector<int> received;
  std::jthread consumer([&] {
    while (auto item = queue.pop()) received.push_back(*item);
  });
  checks.expect(queue.push(1) && queue.push(2) && queue.push(3), "producers make progress");
  queue.close();
  consumer.join();
  checks.expect(received == std::vector<int>({1, 2, 3}), "close drains queued work in order");
  checks.expect(!queue.push(4), "push fails after close");

  constexpr int producer_count = 4;
  constexpr int consumer_count = 4;
  constexpr int items_per_producer = 250;
  netforge::BlockingQueue<int> stress_queue(8);
  std::atomic<int> consumed_count{0};
  std::atomic<long long> consumed_sum{0};
  std::vector<std::jthread> consumers;
  for (int index = 0; index < consumer_count; ++index) {
    consumers.emplace_back([&] {
      while (const auto value = stress_queue.pop()) {
        consumed_count.fetch_add(1, std::memory_order_relaxed);
        consumed_sum.fetch_add(*value, std::memory_order_relaxed);
      }
    });
  }
  std::atomic<bool> producer_failed{false};
  std::vector<std::jthread> producers;
  for (int producer = 0; producer < producer_count; ++producer) {
    producers.emplace_back([&, producer] {
      for (int item = 1; item <= items_per_producer; ++item) {
        const int value = producer * items_per_producer + item;
        if (!stress_queue.push(value)) {
          producer_failed.store(true, std::memory_order_relaxed);
          return;
        }
      }
    });
  }
  producers.clear();
  stress_queue.close();
  consumers.clear();
  constexpr int total_items = producer_count * items_per_producer;
  constexpr long long expected_sum =
      static_cast<long long>(total_items) * (total_items + 1) / 2;
  checks.expect(!producer_failed.load(std::memory_order_relaxed),
                "all producers finish before queue close");
  checks.expect(consumed_count.load(std::memory_order_relaxed) == total_items &&
                    consumed_sum.load(std::memory_order_relaxed) == expected_sum,
                "many producers and consumers neither lose nor duplicate values");

  netforge::BlockingQueue<int> blocked_producer_queue(1);
  checks.expect(blocked_producer_queue.push(1), "capacity-one queue accepts its first item");
  std::promise<void> producer_entered;
  auto producer_entered_future = producer_entered.get_future();
  std::atomic<bool> blocked_push_result{true};
  std::jthread blocked_producer([&] {
    producer_entered.set_value();
    blocked_push_result.store(blocked_producer_queue.push(2), std::memory_order_relaxed);
  });
  producer_entered_future.wait();
  std::this_thread::sleep_for(5ms);
  blocked_producer_queue.close();
  blocked_producer.join();
  checks.expect(!blocked_push_result.load(std::memory_order_relaxed),
                "close wakes a producer blocked on a full queue");

  netforge::BlockingQueue<int> blocked_consumer_queue(1);
  std::promise<void> consumer_entered;
  auto consumer_entered_future = consumer_entered.get_future();
  std::optional<int> blocked_pop_result;
  std::jthread blocked_consumer([&] {
    consumer_entered.set_value();
    blocked_pop_result = blocked_consumer_queue.pop();
  });
  consumer_entered_future.wait();
  std::this_thread::sleep_for(5ms);
  blocked_consumer_queue.close();
  blocked_consumer.join();
  checks.expect(!blocked_pop_result.has_value(),
                "close wakes a consumer blocked on an empty queue");
  return checks.failures;
}

int day05() {
  Checks checks;
  std::atomic<int> total{0};
  netforge::ThreadPool pool(4, 16);
  for (int value = 1; value <= 100; ++value) {
    checks.expect(pool.submit([&total, value] { total.fetch_add(value, std::memory_order_relaxed); }),
                  "task accepted");
  }
  checks.expect(pool.submit([] { throw std::runtime_error("injected task failure"); }),
                "throwing task is accepted");
  pool.shutdown();
  checks.expect(total.load() == 5050, "shutdown joins workers after draining tasks");
  checks.expect(pool.submitted() == 101 && pool.completed() == 101,
                "worker catches a task exception and continues accounting");
  checks.expect(!pool.submit([] {}), "submission fails after shutdown");
  pool.shutdown();
  checks.expect(pool.completed() == 101, "shutdown is idempotent");

  netforge::ThreadPool racing_pool(2, 4);
  std::atomic<int> submitters_ready{0};
  std::atomic<bool> begin_submit{false};
  std::atomic<int> accepted{0};
  std::atomic<int> executed{0};
  std::vector<std::jthread> submitters;
  for (int index = 0; index < 4; ++index) {
    submitters.emplace_back([&] {
      submitters_ready.fetch_add(1, std::memory_order_release);
      while (!begin_submit.load(std::memory_order_acquire)) std::this_thread::yield();
      for (int attempt = 0; attempt < 100; ++attempt) {
        const bool submitted = racing_pool.submit([&] {
          std::this_thread::sleep_for(200us);
          executed.fetch_add(1, std::memory_order_relaxed);
        });
        if (!submitted) break;
        accepted.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  while (submitters_ready.load(std::memory_order_acquire) != 4) std::this_thread::yield();
  begin_submit.store(true, std::memory_order_release);
  std::this_thread::sleep_for(5ms);
  racing_pool.shutdown();
  submitters.clear();
  racing_pool.shutdown();
  checks.expect(accepted.load(std::memory_order_relaxed) > 0,
                "submit/shutdown race accepts some bounded work");
  checks.expect(executed.load(std::memory_order_relaxed) ==
                    accepted.load(std::memory_order_relaxed),
                "every task accepted during concurrent shutdown is drained exactly once");
  checks.expect(racing_pool.submitted() == racing_pool.completed() &&
                    racing_pool.completed() ==
                        static_cast<std::uint64_t>(executed.load(std::memory_order_relaxed)),
                "counters remain coherent across concurrent submit and shutdown");
  return checks.failures;
}

int day06() {
  Checks checks;
  const auto packet = netforge::parse_hex_bytes(
      "000000000000 000000000000 0800 "
      "4500 0028 0000 0000 4006 0000 c0000201 c6336402 "
      "3039 01bb 00000000 00000000 5012 4000 0000 0000");
  const auto summary = netforge::decode_ethernet_ipv4_tcp(packet);
  checks.expect(summary.valid, "well-formed Ethernet/IPv4/TCP packet decodes");
  checks.expect(summary.source_ip == "192.0.2.1" && summary.destination_ip == "198.51.100.2",
                "IPv4 addresses decode in network order");
  checks.expect(summary.source_port == 12345 && summary.destination_port == 443,
                "TCP ports decode in network order");
  checks.expect(summary.tcp_flags == 0x12, "SYN+ACK flags are preserved");
  return checks.failures;
}

int day07() {
  Checks checks;
  netforge::Server server({});
  if (!server.ready() || server.port() == 0) {
    checks.expect(false, "server binds an ephemeral loopback port");
    return checks.failures;
  }
  std::jthread runner([&] { server.run(); });
  const auto ping = netforge::request("127.0.0.1", server.port(), "PING");
  checks.expect(ping.ok && ping.response == "PONG", "blocking client/server round trip works");
  const auto set = netforge::request("127.0.0.1", server.port(), "SET answer 42");
  const auto get = netforge::request("127.0.0.1", server.port(), "GET answer");
  checks.expect(set.ok && set.response == "OK", "SET succeeds");
  checks.expect(get.ok && get.response == "VALUE 42", "GET observes synchronized state");

  std::atomic<int> successful_clients{0};
  std::vector<std::jthread> clients;
  for (int index = 0; index < 24; ++index) {
    clients.emplace_back([&] {
      const auto response = netforge::request("127.0.0.1", server.port(), "PING");
      if (response.ok && response.response == "PONG") {
        successful_clients.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  clients.clear();
  checks.expect(successful_clients.load(std::memory_order_relaxed) == 24,
                "bounded worker pool serves concurrent loopback clients");
  server.request_stop();
  runner.join();
  return checks.failures;
}

int day08() {
  Checks checks;
  int pair[2]{};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
    checks.expect(false, "socketpair created");
    return checks.failures;
  }
  netforge::UniqueFd left(pair[0]);
  netforge::UniqueFd right(pair[1]);
  const std::array<std::byte, 4> oversized{std::byte{0}, std::byte{1}, std::byte{0}, std::byte{1}};
  checks.expect(netforge::write_all(left.get(), oversized).status == netforge::IoStatus::ok,
                "oversized header sent");
  const auto result = netforge::recv_frame(right.get(), 1024);
  checks.expect(result.status == netforge::IoStatus::too_large,
                "length limit rejects work before allocation");
  close_now(left);
  close_now(right);

  netforge::Server server({});
  if (!server.ready()) {
    checks.expect(false, "persistent-connection server starts");
    return checks.failures;
  }
  std::jthread runner([&] { server.run(); });
  netforge::UniqueFd client(::socket(AF_INET, SOCK_STREAM, 0));
  timeval client_timeout{2, 0};
  if (client) {
    (void)::setsockopt(client.get(), SOL_SOCKET, SO_RCVTIMEO, &client_timeout,
                       sizeof(client_timeout));
    (void)::setsockopt(client.get(), SOL_SOCKET, SO_SNDTIMEO, &client_timeout,
                       sizeof(client_timeout));
  }
#if defined(SO_NOSIGPIPE)
  int no_sigpipe = 1;
  if (client) {
    (void)::setsockopt(client.get(), SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
                       sizeof(no_sigpipe));
  }
#endif
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(server.port());
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  const bool connected = client &&
                         ::connect(client.get(), reinterpret_cast<sockaddr*>(&address),
                                   sizeof(address)) == 0;
  checks.expect(connected, "raw client connects for a persistent framed session");
  if (connected) {
    const auto first_send = netforge::send_frame(client.get(), "PING");
    const auto first_reply = netforge::recv_frame(client.get());
    const auto second_send = netforge::send_frame(client.get(), "STATS");
    const auto second_reply = netforge::recv_frame(client.get());
    checks.expect(first_send.status == netforge::IoStatus::ok &&
                      first_reply.status == netforge::IoStatus::ok &&
                      first_reply.payload == "PONG",
                  "first request succeeds on a persistent connection");
    checks.expect(second_send.status == netforge::IoStatus::ok &&
                      second_reply.status == netforge::IoStatus::ok &&
                      second_reply.payload.starts_with("keys="),
                  "second framed request succeeds on the same connection");
  }
  close_now(client);
  server.request_stop();
  runner.join();
  return checks.failures;
}

int day09() {
  Checks checks;
  int pair[2]{};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
    checks.expect(false, "Unix socketpair created");
    return checks.failures;
  }
  const pid_t child_pid = ::fork();
  if (child_pid < 0) {
    checks.expect(false, "fork succeeds");
    (void)::close(pair[0]);
    (void)::close(pair[1]);
    return checks.failures;
  }
  if (child_pid == 0) {
    (void)::alarm(5);
    (void)::close(pair[0]);
    netforge::UniqueFd endpoint(pair[1]);
    const auto request = netforge::recv_frame(endpoint.get());
    const bool replied = request.status == netforge::IoStatus::ok &&
                         netforge::send_frame(endpoint.get(), "ACK " + request.payload).status ==
                             netforge::IoStatus::ok;
    close_now(endpoint);
    ::_exit(replied ? 0 : 11);
  }

  (void)::close(pair[1]);
  netforge::UniqueFd parent(pair[0]);
  checks.expect(netforge::send_frame(parent.get(), "control").status == netforge::IoStatus::ok,
                "parent sends IPC frame");
  const auto reply = netforge::recv_frame(parent.get());
  checks.expect(reply.status == netforge::IoStatus::ok && reply.payload == "ACK control",
                "framing round-trips across a real forked process boundary");
  close_now(parent);

  int child_status = 0;
  pid_t waited = -1;
  do {
    waited = ::waitpid(child_pid, &child_status, 0);
  } while (waited < 0 && errno == EINTR);
  checks.expect(waited == child_pid && WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0,
                "parent reaps the child and observes successful protocol exit");

  int torn_pair[2]{};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, torn_pair) != 0) {
    checks.expect(false, "torn-frame socketpair created");
    return checks.failures;
  }
  const pid_t torn_child_pid = ::fork();
  if (torn_child_pid < 0) {
    checks.expect(false, "torn-frame fork succeeds");
    (void)::close(torn_pair[0]);
    (void)::close(torn_pair[1]);
    return checks.failures;
  }
  if (torn_child_pid == 0) {
    (void)::close(torn_pair[0]);
    netforge::UniqueFd endpoint(torn_pair[1]);
    const auto complete_frame = netforge::encode_frame("partial");
    const auto torn_prefix =
        std::span<const std::byte>(complete_frame.data(), complete_frame.size()).first(6);
    const bool wrote_prefix =
        netforge::write_all(endpoint.get(), torn_prefix).status == netforge::IoStatus::ok;
    close_now(endpoint);
    ::_exit(wrote_prefix ? 0 : 12);
  }

  (void)::close(torn_pair[1]);
  netforge::UniqueFd torn_parent(torn_pair[0]);
  const auto torn_reply = netforge::recv_frame(torn_parent.get());
  checks.expect(torn_reply.status == netforge::IoStatus::eof,
                "parent reports EOF when a child exits halfway through a frame");
  close_now(torn_parent);
  int torn_child_status = 0;
  do {
    waited = ::waitpid(torn_child_pid, &torn_child_status, 0);
  } while (waited < 0 && errno == EINTR);
  checks.expect(waited == torn_child_pid && WIFEXITED(torn_child_status) &&
                    WEXITSTATUS(torn_child_status) == 0,
                "parent reaps a child that exits after a torn frame");
  return checks.failures;
}

int day10() {
  Checks checks;
  std::array<std::byte, 4> bytes{};
  const auto invalid_read = netforge::read_exact(-1, bytes);
  checks.expect(invalid_read.status == netforge::IoStatus::error && invalid_read.error_number == EBADF,
                "syscall failures preserve errno for diagnosis");
  netforge::KvStore store;
  store.set("a", "1");
  checks.expect(store.get("a") == std::optional<std::string>("1"),
                "debug baseline remains deterministic");
  return checks.failures;
}

int day11() {
  Checks checks;
  const auto path = temporary_path("wal");
  {
    netforge::Wal wal(path);
    checks.expect(wal.enabled(), "WAL opens");
    checks.expect(wal.append_set("alpha", "one") && wal.append_set("beta", "two") &&
                      wal.append_del("beta"),
                  "durable records append");
  }
  netforge::KvStore recovered;
  netforge::Wal reader(path);
  checks.expect(reader.recover(recovered), "WAL recovers");
  checks.expect(recovered.get("alpha") == std::optional<std::string>("one"),
                "committed value is restored");
  checks.expect(!recovered.get("beta").has_value(), "delete is replayed");
  {
    std::ofstream tail(path, std::ios::binary | std::ios::app);
    tail.put('\0');
    tail.put('\1');
  }
  netforge::KvStore after_torn_tail;
  checks.expect(reader.recover(after_torn_tail), "torn final record is ignored safely");
  checks.expect(reader.append_set("gamma", "three"), "append succeeds after torn tail repair");
  netforge::KvStore after_repair_restart;
  netforge::Wal final_reader(path);
  checks.expect(final_reader.recover(after_repair_restart),
                "a repaired WAL remains readable after another restart");
  checks.expect(after_repair_restart.get("alpha") == std::optional<std::string>("one") &&
                    after_repair_restart.get("gamma") == std::optional<std::string>("three"),
                "records before and after a repaired tail are retained");
  std::filesystem::remove(path);
  return checks.failures;
}

int day12() {
  Checks checks;
  checks.expect(!netforge::valid_key("../escape"), "path traversal key rejected");
  checks.expect(!netforge::valid_key(std::string(129, 'x')), "key length bounded");
  checks.expect(std::holds_alternative<netforge::ParseError>(netforge::parse_command("")),
                "empty command rejected");
  checks.expect(std::holds_alternative<netforge::ParseError>(
                    netforge::parse_command(std::string("GET a\0hidden", 12))),
                "embedded NUL rejected");
  const std::vector<std::byte> truncated(10);
  checks.expect(!netforge::decode_ethernet_ipv4_tcp(truncated).valid,
                "packet decoder checks every boundary");
  const auto fragmented = netforge::parse_hex_bytes(
      "000000000000 000000000000 0800 "
      "4500 0028 0000 2000 4006 0000 c0000201 c6336402 "
      "3039 01bb 00000000 00000000 5012 4000 0000 0000");
  checks.expect(!netforge::decode_ethernet_ipv4_tcp(fragmented).valid,
                "fragmented IPv4 is not misread as a complete TCP header");
  return checks.failures;
}

int day14() {
  Checks checks;
  {
    netforge::ServerOptions options;
    options.max_payload = netforge::kDefaultMaxPayload + 1U;
    netforge::Server server(options);
    checks.expect(!server.ready(), "server rejects a payload limit incompatible with recovery");
  }
  const auto path = temporary_path("integration-wal");
  {
    netforge::ServerOptions options;
    options.wal_path = path;
    netforge::Server server(options);
    checks.expect(server.ready(), "durability server starts");
    if (server.ready()) {
      std::jthread runner([&] { server.run(); });
      const auto set = netforge::request("127.0.0.1", server.port(), "SET durable survives");
      checks.expect(set.ok && set.response == "OK", "write acknowledged after WAL sync");

      std::atomic<int> durable_writes{0};
      std::vector<std::jthread> writers;
      for (int index = 0; index < 8; ++index) {
        writers.emplace_back([&, index] {
          const auto key = "key" + std::to_string(index);
          const auto value = "value" + std::to_string(index);
          const auto response =
              netforge::request("127.0.0.1", server.port(), "SET " + key + " " + value);
          if (response.ok && response.response == "OK") {
            durable_writes.fetch_add(1, std::memory_order_relaxed);
          }
        });
      }
      writers.clear();
      checks.expect(durable_writes.load(std::memory_order_relaxed) == 8,
                    "concurrent mutations are serialized through the durability boundary");
      server.request_stop();
      runner.join();
    }
  }
  {
    netforge::ServerOptions options;
    options.wal_path = path;
    netforge::Server server(options);
    checks.expect(server.ready(), "recovery server starts");
    if (server.ready()) {
      std::jthread runner([&] { server.run(); });
      const auto get = netforge::request("127.0.0.1", server.port(), "GET durable");
      checks.expect(get.ok && get.response == "VALUE survives", "restart recovers state");
      bool recovered_all = true;
      for (int index = 0; index < 8; ++index) {
        const auto key = "key" + std::to_string(index);
        const auto value = "value" + std::to_string(index);
        const auto response = netforge::request("127.0.0.1", server.port(), "GET " + key);
        recovered_all = recovered_all && response.ok && response.response == "VALUE " + value;
      }
      checks.expect(recovered_all,
                    "restart recovers every concurrently acknowledged mutation");
      server.request_stop();
      runner.join();
    }
  }
  std::filesystem::remove(path);
  return checks.failures;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: netforge-tests dayNN\n";
    return 2;
  }
  const std::string group = argv[1];
  int failures = 0;
  if (group == "day01") failures = day01();
  else if (group == "day02") failures = day02();
  else if (group == "day03") failures = day03();
  else if (group == "day04") failures = day04();
  else if (group == "day05") failures = day05();
  else if (group == "day06") failures = day06();
  else if (group == "day07") failures = day07();
  else if (group == "day08") failures = day08();
  else if (group == "day09") failures = day09();
  else if (group == "day10") failures = day10();
  else if (group == "day11") failures = day11();
  else if (group == "day12") failures = day12();
  else if (group == "day14") failures = day14();
  else {
    std::cerr << "unknown test group: " << group << '\n';
    return 2;
  }
  if (failures == 0) std::cout << group << ": PASS\n";
  return failures == 0 ? 0 : 1;
}
