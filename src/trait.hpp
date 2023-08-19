#include <iostream>
#include <tuple>
#include <type_traits>
#include <variant>

namespace play {
  template <class... Ts>
  struct my_common_type {};

  template <class T>
  struct my_common_type<T> {
    using type = T;
  };

  template <class T1, class T2>
  struct my_common_type<T1, T2> {
    using type = decltype(0 ? std::declval<T1>() : std::declval<T2>());
  };

  template <class T1, class T2, class... Ts>
  struct my_common_type<T1, T2, Ts...> {
    using type = typename my_common_type<T1, typename my_common_type<T2, Ts...>::type>::type;
  };

  template <class T>
  struct tuple_size {};

  template <class... Ts>
  struct tuple_size<std::tuple<Ts...>> {
    static constexpr std::size_t value = sizeof...(Ts);
  };

  template <template <class... Ts> class Tpl, class Tup>
  struct tuple_apply {};

  template <template <class... Ts> class Tpl, class... Ts>
  struct tuple_apply<Tpl, std::tuple<Ts...>> {
    using type = Tpl<Ts...>;
  };

  template <std::size_t N>
  struct array_wrapper {
    template <class T>
    struct rebind {
      using type = std::array<T, N>;
    }
  };

  template <class TplStruct, class Tup>
  struct tuple_map {};

  template <class TplStruct, class... Ts>
  struct tuple_map<TplStruct, std::tuple<Ts...>> {
    using type = std::tuple<typename TplStruct::template rebind<Ts>::type...>;
  };

  using what = tuple_apply<std::variant, std::tuple<int, double>>::type;
  using what_s = tuple_map<array_wrapper<3>, std::tuple<int, double>>::type;
} // namespace play