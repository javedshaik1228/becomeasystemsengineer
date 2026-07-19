#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

int reverse_lines(const std::filesystem::path& input_path,
                  const std::filesystem::path& output_path) {
  std::ifstream input(input_path, std::ios::binary);
  if (!input) {
    std::cerr << "cannot open input: " << input_path << '\n';
    return 1;
  }

  std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
  if (!output) {
    std::cerr << "cannot open output: " << output_path << '\n';
    return 1;
  }

  // TODO: Implement the byte/newline contract from README.md.
  // TODO: Check every read, write, and final flush/error state.
  // TODO: Write down the peak-memory model before optimizing.

  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: reverse-lines INPUT_FILE OUTPUT_FILE\n";
    return 2;
  }

  const std::filesystem::path input_path(argv[1]);
  const std::filesystem::path output_path(argv[2]);
  if (std::filesystem::absolute(input_path).lexically_normal() ==
      std::filesystem::absolute(output_path).lexically_normal()) {
    std::cerr << "input and output must be different paths\n";
    return 2;
  }

  return reverse_lines(input_path, output_path);
}
