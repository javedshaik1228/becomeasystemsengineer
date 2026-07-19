#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

int main() {
  int endpoints[2]{-1, -1};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, endpoints) != 0) {
    std::cerr << "socketpair failed: " << std::strerror(errno) << '\n';
    return 1;
  }

  std::atomic<bool> worker_entered{false};
  std::thread worker([&] {
    worker_entered.store(true, std::memory_order_release);
    std::cout << "worker: blocked waiting for a request\n" << std::flush;
    char byte = 0;
    const auto count = ::recv(endpoints[0], &byte, 1, 0);
    std::cout << "worker: recv returned " << count << '\n' << std::flush;
    (void)::close(endpoints[0]);
  });

  while (!worker_entered.load(std::memory_order_acquire)) std::this_thread::yield();
  std::cout << "main: shutdown requested; joining worker\n" << std::flush;
  worker.join();

  (void)::close(endpoints[1]);
  std::cout << "shutdown: PASS\n";
  return 0;
}

