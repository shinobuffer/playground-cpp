#pragma once
#include <cstddef>

namespace play {
template <size_t I, typename... Ts>
struct nth_type {};

template <typename T, typename... Rest>
struct nth_type<0, T, Rest...> {
  using type = T;
};

template <size_t I, typename T, typename... Rest>
struct nth_type<I, T, Rest...> {
  using type = typename nth_type<I - 1, Rest...>::type;
};

template <size_t I, typename... Ts>
using nth_type_t = typename nth_type<I, Ts...>::type;
} // namespace play
