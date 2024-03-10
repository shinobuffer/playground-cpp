#pragma once

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

namespace play {
namespace _print_details {

#define DEF_SFINAE_VALUE(x) \
  template <class T>        \
  inline constexpr bool x##_v = x<T>::value;

// 移除给定类型的引用和 const 修饰
template <class T>
using _rmcvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class T, class = void>
struct _if_impl_toString : std::false_type {};

template <class T>
struct _if_impl_toString<T, std::void_t<decltype(std::declval<T const&>().toString())>> : std::true_type {};
DEF_SFINAE_VALUE(_if_impl_toString)

template <class T, class = void>
struct _if_impl_stream_insert : std::false_type {};

template <class T>
struct _if_impl_stream_insert<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T const&>())>>
    : std::true_type {};
DEF_SFINAE_VALUE(_if_impl_stream_insert)

template <class T, class = void>
struct _is_tuple_like : std::false_type {};

template <class T>
struct _is_tuple_like<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type {};
DEF_SFINAE_VALUE(_is_tuple_like)

template <class T>
struct _is_optional : std::false_type {};

template <class T>
struct _is_optional<std::optional<T>> : std::true_type {};
DEF_SFINAE_VALUE(_is_optional)

template <class T>
struct _is_variant : std::false_type {};

template <class... Ts>
struct _is_variant<std::variant<Ts...>> : std::true_type {};
DEF_SFINAE_VALUE(_is_variant)

template <class T, class = void>
struct _is_map : std::false_type {};

template <class T>
struct _is_map<T, std::enable_if_t<std::is_same_v<typename T::value_type,
                                                  std::pair<typename T::key_type const, typename T::mapped_type>>>>
    : std::true_type {};
DEF_SFINAE_VALUE(_is_map)

template <class T, class = void>
struct _is_iterable : std::false_type {};

template <class T>
struct _is_iterable<
    T, std::void_t<typename std::iterator_traits<decltype(std::begin(std::declval<T const&>()))>::value_type>>
    : std::true_type {};
DEF_SFINAE_VALUE(_is_iterable)

template <class T>
struct _is_char : std::false_type {};

template <>
struct _is_char<char> : std::true_type {};
DEF_SFINAE_VALUE(_is_char)

template <class T, class = void>
struct _is_string : std::false_type {};

template <class T, class Alloc, class Traits>
struct _is_string<std::basic_string<T, Alloc, Traits>, std::enable_if_t<_is_char_v<T>>> : std::true_type {};

template <class T, class Traits>
struct _is_string<std::basic_string_view<T, Traits>, std::enable_if_t<_is_char_v<T>>> : std::true_type {};
DEF_SFINAE_VALUE(_is_string)

template <class T, class = void>
struct _is_c_str : std::false_type {};

template <class T>
struct _is_c_str<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>> &&
                                     _is_char_v<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>>>>
    : std::true_type {};
DEF_SFINAE_VALUE(_is_c_str)

template <class T>
struct _is_str_like : std::disjunction<_is_string<T>, _is_c_str<T>> {};
DEF_SFINAE_VALUE(_is_str_like)

// 兜底方案（如果可以的话，优先使用流格式化，否则使用地址替代）
template <class T, class = void>
struct _serializer {
  static std::string toString(T const& t) {
    std::ostringstream oss;
    if constexpr (_if_impl_stream_insert_v<T>) {
      oss << t;
      return oss.str();
    }
    oss << std::hex << std::showbase << "addr("
        << reinterpret_cast<std::size_t>(reinterpret_cast<void const volatile*>(std::addressof(t))) << ")";
    return oss.str();
  }
};

// 如果实现了 toString，直接调用
template <class T>
struct _serializer<T, std::enable_if_t<_if_impl_toString_v<T>>> {
  static std::string toString(T const& t) { return t.toString(); }
};

// 可迭代对象（字符串、map、以及递归类型除外）特化实现
template <class T>
struct _serializer<
    T, std::enable_if_t<
           !_if_impl_toString_v<T> && _is_iterable_v<T> &&
           !std::is_same_v<typename std::iterator_traits<decltype(std::begin(std::declval<T const&>()))>::value_type,
                           std::decay_t<T>> &&
           !_is_str_like_v<T> && !_is_map_v<T>>> {
  static std::string toString(T const& t) {
    std::ostringstream oss;
    oss << "[";
    bool flag = false;
    for (auto const& item : t) {
      if (flag) {
        oss << ", ";
      } else {
        flag = true;
      }
      oss << _serializer<_rmcvref_t<decltype(item)>>::toString(item);
    }
    oss << "]";
    return oss.str();
  }
};

// map 特化实现
template <class T>
struct _serializer<T, std::enable_if_t<!_if_impl_toString_v<T> && _is_map_v<T>>> {
  static std::string toString(T const& t) {
    std::ostringstream oss;
    oss << "{";
    bool flag = false;
    for (auto const& [key, value] : t) {
      if (flag) {
        oss << ", ";
      } else {
        flag = true;
      }
      oss << _serializer<_rmcvref_t<decltype(key)>>::toString(key);
      oss << ": ";
      oss << _serializer<_rmcvref_t<decltype(value)>>::toString(value);
    }
    oss << "}";
    return oss.str();
  }
};

// tuple/pair 特化实现
template <class T>
struct _serializer<T, std::enable_if_t<!_if_impl_toString_v<T> && _is_tuple_like_v<T>>> {
  template <std::size_t... Idx>
  static std::string unroll(T const& t, std::index_sequence<Idx...>) {
    std::ostringstream oss;
    oss << "{";
    ((oss << (Idx == 0 ? "" : ", ")
          << _serializer<_rmcvref_t<std::tuple_element_t<Idx, T>>>::toString(std::get<Idx>(t))),
     ...);
    oss << "}";
    return oss.str();
  }

  static std::string toString(T const& t) { return unroll(t, std::make_index_sequence<std::tuple_size_v<T>>{}); }
};

// 字符串特化实现
template <class T>
struct _serializer<T, std::enable_if_t<!_if_impl_toString_v<T> && _is_str_like_v<T>>> {
  static std::string toString(T const& t) {
    std::ostringstream oss;
    oss << std::quoted(t);
    return oss.str();
  }
};

// 字符特化实现
template <class T>
struct _serializer<T, std::enable_if_t<!_if_impl_toString_v<T> && _is_char_v<T>>> {
  static std::string toString(T const& t) {
    std::ostringstream oss;
    T res[2] = {t, T('\0')};
    oss << std::quoted(res, '\'');
    return oss.str();
  }
};

template <>
struct _serializer<bool, void> {
  static std::string toString(bool v) { return v ? "true" : "false"; };
};

template <>
struct _serializer<std::nullptr_t, void> {
  static std::string toString(std::nullptr_t const&) { return "nullptr"; };
};

template <>
struct _serializer<std::nullopt_t, void> {
  static std::string toString(std::nullopt_t const&) { return "nullopt"; };
};

// optional 特化实现
template <class T>
struct _serializer<T, std::enable_if_t<!_if_impl_toString_v<T> && _is_optional_v<T>>> {
  static std::string toString(T const& t) {
    if (t) {
      return "opt(" + _serializer<typename T::value_type>::toString(*t) + ")";
    }
    return _serializer<std::nullopt_t>::toString(std::nullopt);
  }
};

// variant 特化实现
template <class T>
struct _serializer<T, std::enable_if_t<!_if_impl_toString_v<T> && _is_variant_v<T>>> {
  static std::string toString(T const& t) {
    std::string inner =
        std::visit([](auto const& v) -> std::string { return _serializer<_rmcvref_t<decltype(v)>>::toString(v); }, t);
    return "var(" + inner + ")";
  }
};

template <class T, class... Ts>
std::string toString(T const& t, Ts const&... ts) {
  std::string res = _serializer<_rmcvref_t<T>>::toString(t);
  ((res += " ", res.append(_serializer<_rmcvref_t<Ts>>::toString(ts))), ...);
  return res;
}

// 将内容输出至指定流
template <class T, class... Ts>
void oprint(std::ostream& os, T const& t, Ts const&... ts) {
  os << toString(t, ts...);
}

// 将内容输出至指定流（自带换行）
template <class T, class... Ts>
void oprintln(std::ostream& os, T const& t, Ts const&... ts) {
  os << toString(t, ts...).append("\n");
}

// 将内容输出至标准输出流
template <class T, class... Ts>
void print(T const& t, Ts const&... ts) {
  oprint(std::cout, t, ts...);
}

// 将内容输出至标准输出流（自带换行）
template <class T, class... Ts>
void println(T const& t, Ts const&... ts) {
  oprintln(std::cout, t, ts...);
}

} // namespace _print_details

using _print_details::oprint;
using _print_details::oprintln;
using _print_details::print;
using _print_details::println;
using _print_details::toString;

} // namespace play
