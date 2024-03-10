#pragma once
// impl under C++14
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
  using type = typename nth_type<I, Ts...>::type;
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

  // 供非平凡拷贝构造函数进一步判断是否会抛异常
  static constexpr bool is_nothrow_default_ctor = std::is_nothrow_default_constructible_v<nth_type_t<0, Ts...>>;
  static constexpr bool is_nothrow_copy_ctor = false;
  static constexpr bool is_nothrow_move_ctor = (std::is_nothrow_move_constructible_v<Ts> && ...);
  static constexpr bool is_nothrow_copy_assign = false;
  static constexpr bool is_nothrow_move_assign = (std::is_nothrow_move_assignable_v<Ts> && ...);
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
private:
  size_t m_index;
};

} // namespace _variant_detail

} // namespace play
