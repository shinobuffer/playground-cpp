#include "demangle.hpp"

#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char const* argv[]) {
  auto str = std::string("114514");
  auto int_num = 114514;
  std::cout << play::demangle<decltype(str)>() << "\n" << play::demangle<decltype((int_num))>() << "\n";
  return 0;
}
