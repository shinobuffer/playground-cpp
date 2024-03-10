#include "print.hpp"

#include <unordered_map>

int main([[maybe_unused]] int argc, [[maybe_unused]] char const* argv[]) {
  auto map = std::unordered_map<std::string, std::variant<int, double>>({{"k1", 114}, {"k2", 514.810}});
  play::println(map);
  return 0;
}
