#pragma once
// impl under C++17
#include "trait.hpp"
#include <functional>
#include <type_traits>
#include <utility>

namespace play {
template <typename... Ts>
class variant;

inline constexpr size_t variant_npos = -1;

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
  using type = nth_type_t<I, Ts...>;
};

template <size_t I, typename V>
using variant_alternative_t = typename variant_alternative<I, V>::type;

template <size_t I, typename V>
struct variant_alternative<I, const V> : std::add_const<variant_alternative_t<I, V>> {};

template <size_t I, typename V>
struct variant_alternative<I, volatile V> : std::add_volatile<variant_alternative_t<I, V>> {};

template <size_t I, typename V>
struct variant_alternative<I, const volatile V> : std::add_cv<variant_alternative_t<I, V>> {};

namespace _variant_detail {

// tag for raw visit
struct variant_cookie {};
// tag for raw visit with with indices passed in
struct variant_idx_cookie {
  using type = variant_idx_cookie;
};
// tag for enable deduction (and same-type checking) for visit
template <typename T>
struct deduce_visit_result {
  using type = T;
};

template <typename... Ts>
struct traits {
  static constexpr bool is_default_ctor = std::is_default_constructible_v<nth_type<0, Ts...>>;
  static constexpr bool is_copy_ctor = (std::is_copy_constructible_v<Ts> && ...);
  static constexpr bool is_move_ctor = (std::is_move_constructible_v<Ts> && ...);
  static constexpr bool is_copy_assign = is_copy_ctor && (std::is_copy_assignable_v<Ts> && ...);
  static constexpr bool is_move_assign = is_move_ctor && (std::is_move_assignable_v<Ts> && ...);

  static constexpr bool is_trivial_dtor = (std::is_trivially_destructible_v<Ts> && ...);
  static constexpr bool is_trivial_copy_ctor = (std::is_trivially_copy_constructible_v<Ts> && ...);
  static constexpr bool is_trivial_move_ctor = (std::is_trivially_move_constructible_v<Ts> && ...);
  static constexpr bool is_trivial_copy_assign =
      is_trivial_copy_ctor && (std::is_trivially_copy_assignable_v<Ts> && ...);
  static constexpr bool is_trivial_move_assign =
      is_trivial_move_ctor && (std::is_trivially_move_assignable_v<Ts> && ...);

  static constexpr bool is_nothrow_default_ctor = std::is_nothrow_default_constructible_v<nth_type_t<0, Ts...>>;
  static constexpr bool is_nothrow_copy_ctor = false;
  static constexpr bool is_nothrow_move_ctor = (std::is_nothrow_move_constructible_v<Ts> && ...);
  static constexpr bool is_nothrow_copy_assign = false;
  static constexpr bool is_nothrow_move_assign = (std::is_nothrow_move_assignable_v<Ts> && ...);
};

// Suitably-small, trivially copyable types can always be placed in a variant without it becoming valueless
template <typename T>
struct never_valueless_alt : std::conjunction<std::bool_constant<sizeof(T) <= 256>, std::is_trivially_copyable<T>> {};

template <typename... Ts>
struct never_valueless
    : std::conjunction<std::bool_constant<traits<Ts...>::is_move_assign>, never_valueless_alt<Ts>...> {};

// TRICK
// 提取基础类型的小技巧：
// 第三个模板参数被特地设置为了第二个参数移除 cv 和 ref 的结果
// 这样在特化第三个参数时填充基础类型，第二个参数就能自动匹配 const 和 volatile 的组合，实际上使用的是第三参数
template <typename Maybe_variant_cooke, typename V, typename = std::remove_cv_t<std::remove_reference_t<V>>>
inline constexpr bool extra_visit_slot_needed = false;

template <typename V, typename... Ts>
inline constexpr bool extra_visit_slot_needed<variant_cookie, V, variant<Ts...>> = !never_valueless<Ts...>::value;

template <typename V, typename... Ts>
inline constexpr bool extra_visit_slot_needed<variant_idx_cookie, V, variant<Ts...>> = !never_valueless<Ts...>::value;

template <typename T, size_t... Dimensions>
struct multi_array;

template <typename T>
struct multi_array<T> {
  template <typename>
  struct untag_result : std::false_type {
    using element_type = T;
  };

  template <typename... Args>
  struct untag_result<const void (*)(Args...)> : std::false_type {
    using element_type = void (*)(Args...);
  };

  template <typename... Args>
  struct untag_result<variant_cookie (*)(Args...)> : std::false_type {
    using element_type = void (*)(Args...);
  };

  template <typename... Args>
  struct untag_result<variant_idx_cookie (*)(Args...)> : std::false_type {
    using element_type = void (*)(Args...);
  };

  template <typename Ret, typename... Args>
  struct untag_result<deduce_visit_result<Ret> (*)(Args...)> : std::true_type {
    using element_type = Ret (*)(Args...);
  };

  static constexpr bool result_is_deduced = untag_result<T>::value;

  typename untag_result<T>::element_type m_data;

  constexpr const typename untag_result<T>::element_type& access() const { return m_data; }
};

template <typename Ret, typename Visitor, typename... Vs, size_t first, size_t... rest>
struct multi_array<Ret (*)(Visitor, Vs...), first, rest...> {
  using V = nth_type_t<sizeof...(Vs) - sizeof...(rest) - 1, Vs...>;

  static constexpr int do_cookie = extra_visit_slot_needed<Ret, V> ? 1 : 0;
  using T = Ret (*)(Visitor, Vs...);

  multi_array<T, rest...> m_arr[first + do_cookie];

  template <typename... Idxs>
  constexpr decltype(auto) access(size_t first_index, Idxs... rest_indices) const {
    return m_arr[first_index + do_cookie].access(rest_indices...);
  }
};

template <typename Array_type, typename Idx_seq>
struct gen_vtable_impl;

template <typename Ret, typename Visitor, typename... Vs, size_t... Idxs>
struct gen_vtable_impl<multi_array<Ret (*)(Visitor, Vs...)>, std::index_sequence<Idxs...>> {
  /* data */
};

template <typename Ret, typename Visitor, size_t... Dimensions, typename... Vs, size_t... Idxs>
struct gen_vtable_impl<multi_array<Ret (*)(Visitor, Vs...), Dimensions...>, std::index_sequence<Idxs...>> {
  using Array_type = multi_array<Ret (*)(Visitor, Vs...), Dimensions...>;
};

template <typename Ret, typename Visitor, typename... Vs>
struct gen_vtable {
  // TODO
};

template <typename Ret, typename Visitor, typename... Vs>
constexpr decltype(auto) doVisit(Visitor&& visitor, Vs&&... variants) {
  // early return for case of visiting no variants
  if constexpr (sizeof...(Vs) == 0) {
    if constexpr (std::is_void_v<Ret>) {
      return (void)std::forward<Visitor>(visitor)();
    }
    return std::forward<Visitor>(visitor)();
  }

  using V0 = nth_type_t<0, Vs...>;
  // too many variants or too many alternatives of the first variant, using a jump table to generate case
  if constexpr (sizeof...(Vs) > 1 || variant_size_v<V0> > 11) {
    // TODO
  }
}

// For C++17: _Uninitialized<T> is guaranteed to be a trivially destructible type, even if T is not.
// For C++20: _Uninitialized<T> is trivially destructible iff T is, so _Variant_union needs a constrained non-trivial
// destructor.
//
// Here is storage for a T, that implicitly begins its lifetime if T is an implicit-lifetime-type, but otherwise will
// not actually initialize it for you - you have to do that yourself. Likewise it will not destroy it for you, you have
// to do that yourself too
template <typename T, bool = std::is_trivially_destructible_v<T>>
class Uninitialized;

template <typename T>
class Uninitialized<T, true> {
private:
  T m_storage;

public:
  template <typename... Args>
  constexpr Uninitialized(in_place_index_t<0>, Args&&... args) : m_storage(std::forward<Args>(args)...) {}

  constexpr const T& get() const& noexcept { return m_storage; }
  constexpr T& get() & noexcept { return m_storage; }
  constexpr const T&& get() const&& noexcept { return std::move(m_storage); }
  constexpr T&& get() && noexcept { return std::move(m_storage); }
};

template <typename T>
class Uninitialized<T, false> {
private:
  alignas(alignof(T)) unsigned char m_storage[sizeof(T)];

public:
  template <typename... Args>
  constexpr Uninitialized(in_place_index_t<0>, Args&&... args) {
    new ((void*)&m_storage) T(std::forward<Args>(args)...);
  }

  const T& get() const& noexcept { return *reinterpret_cast<const T*>(&m_storage); }
  T& get() & noexcept { return *reinterpret_cast<T*>(&m_storage); }
  const T&& get() const&& noexcept { return std::move(*reinterpret_cast<const T*>(&m_storage)); }
  T&& get() && noexcept { return std::move(*reinterpret_cast<T*>(&m_storage)); }
};

template <bool trivially_destructible, typename... Ts>
union VariadicUnion {
  VariadicUnion() = default;

  template <size_t I, typename... Args>
  VariadicUnion(in_place_index_t<I>, Args&&...) = delete;
};

template <bool trivially_destructible, typename First, typename... Rest>
union VariadicUnion<trivially_destructible, First, Rest...> {
private:
  Uninitialized<First> m_first;
  VariadicUnion<trivially_destructible, Rest...> m_rest;

public:
  constexpr VariadicUnion() : m_rest() {}

  template <typename... Args>
  constexpr VariadicUnion(in_place_index_t<0>, Args&&... args)
      : m_first(in_place_index<0>, std::forward<Args>(args)...) {}

  template <size_t I, typename... Args>
  constexpr VariadicUnion(in_place_index_t<I>, Args&&... args)
      : m_rest(in_place_index<I - 1>, std::forward<Args>(args)...) {}
};

template <bool trivially_destructible, typename... Ts>
class VariantStorage;

template <typename... Ts>
class VariantStorage<false, Ts...> {
public:
  // In fact using a smaller unsigned integer type instead of size_t could save more space.
  // In std, they choose to use unsigned char/short base on the size of the parameter pack.
  using index_type = size_t;

private:
  index_type m_index;
  VariadicUnion<false, Ts...> m_u;

public:
  constexpr VariantStorage() : m_index(static_cast<index_type>(variant_npos)) {}

  template <size_t I, typename... Args>
  constexpr VariantStorage(in_place_index_t<I>, Args&&... args)
      : m_u(in_place_index<I>, std::forward<Args>(args)...), m_index(I) {}
};

template <typename... Ts>
class VariantStorage<true, Ts...> {
public:
  using index_type = size_t;

private:
  index_type m_index;
  VariadicUnion<true, Ts...> m_u;

public:
  constexpr VariantStorage() : m_index(static_cast<size_t>(variant_npos)) {}

  template <size_t I, typename... Args>
  constexpr VariantStorage(in_place_index_t<I>, Args&&... args)
      : m_u(in_place_index<I>, std::forward<Args>(args)...), m_index(I) {}
};

} // namespace _variant_detail

} // namespace play
