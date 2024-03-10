#pragma onec

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

} // namespace play
