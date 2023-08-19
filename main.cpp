#include <logger.hpp>

int main(int argc, char const *argv[]) {
  int num = 1;
  LOG_P(num);
  generic_log(LogLevel::Info, "{}", "hello");
  return 0;
}
