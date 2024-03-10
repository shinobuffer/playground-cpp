#pragma onec
// impl under C++14
#include "trait.hpp"
#include <functional>
#include <type_traits>

namespace play {
template <typename... Ts>
class variant;

template <typename V>
struct variant_size;

template <typename... Ts>
struct variant_size<variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template <typename V>
struct variant_size<const V> : variant_size<V> {};

template <typename V>
struct variant_size<volatile V> : variant_size<V> {};

template <typename V>
struct variant_size<const volatile V> : variant_size<V> {};

template <typename V>
inline constexpr size_t variant_size_v = variant_size<V>::value;

template <size_t I, typename V>
struct variant_alternative;

template <size_t I, typename... Ts>
struct variant_alternative<I, variant<Ts...>> {
  static_assert(I < sizeof...(Ts));
  using type = typename nth_type<I, Ts...>::type
};

template <size_t I, typename V>
struct variant_alternative<I, const V> : std::add_const<typename variant_alternative<I, V>::type> {};

template <size_t I, typename V>
struct variant_alternative<I, volatile V> : std::add_volatile<typename variant_alternative<I, V>::type> {};

template <size_t I, typename V>
struct variant_alternative<I, const volatile V> : std::add_cv<typename variant_alternative<I, V>::type> {};

template <size_t I, typename V>
using variant_alternative_t = typename variant_alternative<I, V>::type;

namespace _variant_detail {

template <typename... Ts>
struct traits {
  static constexpr bool is_default_ctor = std::is_default_constructible_v<nth_type<0, Ts...>>;
  static constexpr bool is_copy_ctor = (std::is_copy_constructible_v<Ts> && ...);
  static constexpr bool is_move_ctor = (std::is_move_constructible_v<Ts> && ...);
  static constexpr bool is_copy_assign = is_copy_ctor && (std::is_copy_assignable_v<Ts> && ...);
  static constexpr bool is_move_assign = is_move_ctor && (std::is_move_assignable_v<Ts> && ...);

  static constexpr bool is_trivial_dtor = (std::is_trivially_destructible_v<Ts...> && ...);
  static constexpr bool is_trivial_copy_ctor = (std::is_trivially_copy_constructible_v<Ts> && ...);
  static constexpr bool is_trivial_move_ctor = (std::is_trivially_move_constructible_v<Ts> && ...);
  static constexpr bool is_trivial_copy_assign =
      is_trivial_copy_ctor && (std::is_trivially_copy_assignable_v<Ts> && ...);
  static constexpr bool is_trivial_move_assign =
      is_trivial_move_ctor && (std::is_trivially_move_assignable_v<Ts> && ...);

  // 供非平凡拷贝构造函数进一步判断是否会抛异常
  static constexpr bool is_nothrow_default_ctor = std::is_nothrow_default_constructible_v<nth_type_t<0, Ts...>>;
  static constexpr bool is_nothrow_copy_ctor = false;
  static constexpr bool is_nothrow_move_ctor = (std::is_nothrow_move_constructible_v<Ts> && ...);
  static constexpr bool is_nothrow_copy_assign = false;
  static constexpr bool is_nothrow_move_assign = (std::is_nothrow_move_assignable_v<Ts> && ...);
};

void test() { auto constexpr v = traits<int>::is_nothrow_default_ctor; }

} // namespace _variant_detail

} // namespace play
