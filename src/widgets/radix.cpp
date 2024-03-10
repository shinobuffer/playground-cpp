#include <cxxopts.hpp>
#include <iostream>

int main(int argc, char const *argv[]) {
  cxxopts::Options options("radix-dump", "Show an integer literal in common-used radix, or other specified radix");
  options.add_options()                                      //
      ("b,bin", "Binary representation")                     //
      ("o,oct", "Octal representation")                      //
      ("d,dec", "Decimal representation")                    //
      ("h,hex", "Hexadecimal representation")                //
      ("r,radix", "Specified radix", cxxopts::value<int>())  //
      ("help", "Usage");                                     //

  auto args = options.parse(argc, argv);

  if (args.count("help")) {
    std::cout << options.help() << '\n';
    return 0;
  }

  std::cout << args.arguments_string();

  if (args.count("radix")) {
    auto radix = args["radix"].as<int>();
    std::cout << radix;
    return 0;
  }

  std::cout << args.arguments_string();

  return 0;
}
