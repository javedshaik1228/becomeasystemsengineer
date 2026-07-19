#include "baseline.hpp"

namespace netforge::diagnostic {

HeaderResult inspect_first_frame(std::span<const std::byte>, std::size_t) {
  // Practical diagnostic: implement this function without opening the course
  // reference tree. The test output names the missing behavior.
  return {};
}

}  // namespace netforge::diagnostic
