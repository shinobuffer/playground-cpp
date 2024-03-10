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

template <size_t I, typename V>
struct variant_aliternative;

} // namespace play
