#include "demangle.hpp"
#include "print.hpp"

int main(int argc, char const* argv[]) {
  std::variant<double, std::string> var = "114.514";
  play::oprintln(std::cout, var);
  var = 114.514;
  play::oprintln(std::cout, var);
  return 0;
}
