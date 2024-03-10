#pragma once

#include <cstddef>
#include <type_traits>

namespace play {
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

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

template <typename T>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type{};

template <size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

template <size_t I>
inline constexpr in_place_index_t<I> in_place_index{};
} // namespace play
