#include <thread>

int main() {
  int value = 0;
  std::jthread first([&] {
    for (int index = 0; index < 100'000; ++index) ++value;
  });
  std::jthread second([&] {
    for (int index = 0; index < 100'000; ++index) ++value;
  });
  first.join();
  second.join();
  return value == 200'000 ? 0 : 1;
}

