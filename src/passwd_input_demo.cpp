#include "passwd_input.hpp"

#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char const* argv[]) {
  play::UnixPasswdInput input;
  std::cout << input.request("Please input: ");
  return 0;
}