#pragma once

#include <iostream>
#include <optional>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace _details {
  // 移除给定类型的引用和 const 修饰
  template <class T>
  using _rmcvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

  // 默认实现，直接输出至输出流
  template <class T, class = void>
  struct _printer {
    static void print(std::ostream &os, T const &t) { os << t; }
    using is_default_print = std::true_type;
  };

  // 判断给定类型是否可以直接输出至输出流
  template <class T, class = void>
  struct _is_default_printable : std::false_type {};

  template <class T>
  struct _is_default_printable<T, std::void_t<typename _printer<T>::is_default_print,
                                              decltype(std::declval<std::ostream &>() << std::declval<T const &>())>>
      : std::true_type {};

  template <class T, class = void>
  struct _is_printer_printable : std::true_type {};

  template <class T>
  struct _is_printer_printable<T, std::void_t<typename _printer<T>::is_default_print>> : std::false_type {};

  template <class T>
  struct is_printable : std::disjunction<_is_default_printable<T>, _is_printer_printable<T>> {};

  // 判断给定类型是否为 tuple（如果是则拥有 type 成员，否则拥有 not_type 成员，其他 _enable_if_xxx 模板同理）
  template <class T, class U = void>
  struct _enable_if_tuple {
    using not_type = U;
  };

  template <class... Ts, class U>
  struct _enable_if_tuple<std::tuple<Ts...>, U> {
    using type = U;
  };

  // 判断给定类型是否为 map（通过判断元素类型是否为键类型和值类型组成的 pair）
  template <class T, class U = void, class = void>
  struct _enable_if_map {
    using not_type = U;
  };

  template <class T, class U>
  struct _enable_if_map<T, U,
                        std::enable_if_t<std::is_same_v<
                            typename T::value_type, std::pair<typename T::key_type const, typename T::mapped_type>>>> {
    using type = U;
  };

  // 判断给定类型是否可迭代（通过 std::begin 尝试获取给定类型的迭代器，并利用 std::iterator_traits
  // 判断其是否满足迭代器要求）
  template <class T, class U = void, class = void>
  struct _enable_if_iterable {
    using not_type = U;
  };

  template <class T, class U>
  struct _enable_if_iterable<
      T, U, std::void_t<typename std::iterator_traits<decltype(std::begin(std::declval<T const &>()))>::value_type>> {
    using type = U;
  };

  // 判断给定类型是否实现 doPrint 接口
  template <class T, class U = void, class = void>
  struct _enable_if_has_print {
    using not_type = U;
  };

  template <class T, class U>
  struct _enable_if_has_print<
      T, U, std::void_t<decltype(std::declval<T const &>().doPrint(std::declval<std::ostream &>()))>> {
    using type = U;
  };

  // 判断给定类型是否为字符
  template <class T>
  struct _is_char : std::false_type {};

  template <>
  struct _is_char<char> : std::true_type {};

  template <>
  struct _is_char<wchar_t> : std::true_type {};

  // 判断给定类型是否为字符
  template <class T, class U = void, class = void>
  struct _enable_if_char {
    using not_type = U;
  };

  template <class T, class U>
  struct _enable_if_char<T, U, std::enable_if_t<_is_char<T>::value>> {
    using type = U;
  };

  // 判断给定类型是否为字符串（string/string_view）
  template <class T, class U = void, class = void>
  struct _enable_if_string {
    using not_type = U;
  };

  template <class T, class Alloc, class Traits, class U>
  struct _enable_if_string<std::basic_string<T, Alloc, Traits>, U, typename _enable_if_char<T>::type> {
    using type = U;
  };

  template <class T, class Traits, class U>
  struct _enable_if_string<std::basic_string_view<T, Traits>, U, typename _enable_if_char<T>::type> {
    using type = U;
  };

  // 判断给定类型是否为 C 风格字符串（首先需要是指针，然后判断去除指针、cvr 后的基类型是否为字符）
  template <class T, class U = void, class = void>
  struct _enable_if_c_str {
    using not_type = U;
  };

  template <class T, class U>
  struct _enable_if_c_str<
      T, U,
      std::enable_if_t<std::is_pointer_v<std::decay_t<T>> &&
                       _is_char<std::remove_const_t<std::remove_pointer_t<std::decay_t<T>>>>::value>> {
    using type = U;
  };

  // 判断给定类型是否为 optional
  template <class T, class U = void, class = void>
  struct _enable_if_optional {
    using not_type = U;
  };

  template <class T, class U>
  struct _enable_if_optional<std::optional<T>, U> {
    using type = U;
  };

  // 判断给定类型是否为 variant
  template <class T, class U = void, class = void>
  struct _enable_if_variant {
    using not_type = U;
  };

  template <class... Ts, class U>
  struct _enable_if_variant<std::variant<Ts...>, U> {
    using type = U;
  };

  // 优先：如果对象实现了 doPrint 接口，直接调用
  template <class T>
  struct _printer<T, typename _enable_if_has_print<T>::type> {
    static void print(std::ostream &os, T const &t) { t.doPrint(os); }
  };

  // 如果是可迭代对象（字符串和 map 除外），对其中的每个元素根据其基类型进行输出
  template <class T>
  struct _printer<T, std::void_t<typename _enable_if_has_print<T>::not_type, typename _enable_if_iterable<T>::type,
                                 typename _enable_if_c_str<T>::not_type, typename _enable_if_string<T>::not_type>> {
    static void print(std::ostream &os, T const &t) {
      os << "{";
      bool flag = false;
      for (auto const &item : t) {
        if (flag) {
          os << ", ";
        } else {
          flag = true;
        }
        _printer<_rmcvref_t<decltype(item)>>::print(os, item);
      }
      os << "}";
    }
  };

  template <class T, class... Ts>
  void fprint(std::ostream &os, T const &t, Ts const &...ts) {
    // 提取基类型，根据基类型转发到不同实现上
    _printer<_rmcvref_t<T>>::print(os, t);
    ((os << ' ', _printer<_rmcvref_t<Ts>>::print(os, ts)), ...);
    os << '\n';
  }

} // namespace _details

using ::_details::fprint;
using ::_details::is_printable;