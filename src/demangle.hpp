#pragma once

#include <string>
#include <type_traits>
#if defined(__GUNC__) || defined(__clang__)
#include <cxxabi.h>

#include <cstdlib>
#endif

#include <memory>

namespace play {
  // 将 abi 标识符转换为源码标识符
  inline std::string demangleAbiToken(const char* token) {
#if defined(__GUNC__) || defined(__clang__)
    char* cp = abi::__cxa_demangle(token, NULL, NULL, NULL);
    std::string name(cp ? cp : token);
    std::free(cp);
#else
    std::string name(token);
#endif
    return name;
  }

  // Usage: std::cout << demangle<int* const>(); => int* const
  template <class T>
  std::string demangle() {
    // 基类型
    std::string name(demangleAbiToken(typeid(std::remove_cv_t<std::remove_reference_t<T>>).name()));
    // 追加修饰符
    if constexpr (std::is_const_v<std::remove_reference_t<T>>) name += " const";
    if constexpr (std::is_volatile_v<std::remove_reference_t<T>>) name += " volatile";
    if constexpr (std::is_lvalue_reference_v<T>) name += " &";
    if constexpr (std::is_rvalue_reference_v<T>) name += " &&";

    return name;
  }
} // namespace play
