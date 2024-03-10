#include "variant.hpp"
#include <variant>

int main(int argc, char const* argv[]) {
  std::variant<int, std::nullptr_t> v = nullptr;
  /* code */
  return 0;
}
