#include <cstddef>

int main() {
  auto* values = new int[2]{7, 11};
  delete[] values;
  volatile int observed = values[1];
  return observed == 11 ? 0 : 1;
}

