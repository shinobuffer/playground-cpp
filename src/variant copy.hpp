#pragma once
#include <functional>
#include <type_traits>
#include <utility>

#include "enable_smfs.hpp"
#include "trait.hpp"

namespace play {
template <typename... Ts>
class Variant;

inline constexpr size_t variant_npos = -1;

template <typename V>
struct variant_size;

template <typename... Ts>
struct variant_size<Variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

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
struct variant_alternative<I, Variant<Ts...>> {
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
// Inheritance chain:
// VariantStorage(VariantUnion) -> CopyCtorBase -> MoveCtorBase -> CopyAssignBase -> MoveAssignBase -> Variant
template <bool trivially_dtor, typename... Ts>
struct VariantStorage {};

template <typename... Ts>
using VariantStorageAlias = VariantStorage<traits<Ts...>::is_trivial_dtor, Ts...>;

template <bool trivially_copy_ctor, typename... Ts>
struct CopyCtorBase : VariantStorageAlias<Ts...> {
  using BaseT = VariantStorageAlias<Ts...>;
  using BaseT::BaseT;
};

template <typename... Ts>
struct CopyCtorBase<true, Ts...> : VariantStorageAlias<Ts...> {
  using BaseT = VariantStorageAlias<Ts...>;
  using BaseT::BaseT;
};

template <typename... Ts>
using CopyCtorBaseAlias = CopyCtorBase<traits<Ts...>::is_trivial_copy_ctor, Ts...>;

template <bool trivially_move_ctor, typename... Ts>
struct MoveCtorBase : CopyCtorBaseAlias<Ts...> {
  using BaseT = CopyCtorBaseAlias<Ts...>;
  using BaseT::BaseT;

  MoveCtorBase(MoveCtorBase&&) noexcept(traits<Ts...>::is_nothrow_move_ctor) {
    // TODO
  };

  MoveCtorBase(MoveCtorBase const&) = default;
  MoveCtorBase& operator=(MoveCtorBase const&) = default;
  MoveCtorBase& operator=(MoveCtorBase&&) = default;
};

template <typename... Ts>
struct MoveCtorBase<true, Ts...> : CopyCtorBaseAlias<Ts...> {
  using BaseT = CopyCtorBaseAlias<Ts...>;
  using BaseT::BaseT;
};

template <typename... Ts>
using MoveCtorBaseAlias = MoveCtorBase<traits<Ts...>::is_trivial_move_ctor, Ts...>;

template <bool trivially_copy_assign, typename... Ts>
struct CopyAssignBase : MoveCtorBaseAlias<Ts...> {
  using BaseT = MoveCtorBaseAlias<Ts...>;
  using BaseT::BaseT;

  // If not all alternatives are trivially copy assignable, need to manually invoke copy opeartion
  CopyAssignBase& operator=(CopyAssignBase const&) noexcept(traits<Ts...>::is_nothrow_copy_assign) {
    // TODO
  };

  CopyAssignBase(CopyAssignBase const&) = default;
  CopyAssignBase(CopyAssignBase&&) = default;
  CopyAssignBase& operator=(CopyAssignBase&&) = default;
};

template <typename... Ts>
struct CopyAssignBase<true, Ts...> : MoveCtorBaseAlias<Ts...> {
  using BaseT = MoveCtorBaseAlias<Ts...>;
  using BaseT::BaseT;
};

template <typename... Ts>
using CopyAssignBaseAlias = CopyAssignBase<traits<Ts...>::is_trivial_copy_assign, Ts...>;

template <bool trivially_move_assign, typename... Ts>
struct MoveAssignBase : CopyAssignBaseAlias<Ts...> {
  using BaseT = CopyAssignBaseAlias<Ts...>;
  using BaseT::BaseT;

  // If not all alternatives are trivially move assignable, need to manually invoke move opeartion
  MoveAssignBase& operator=(MoveAssignBase&& rhs) noexcept(traits<Ts...>::is_nothrow_move_assign) {
    // TODO
  }

  MoveAssignBase(MoveAssignBase const&) = default;
  MoveAssignBase(MoveAssignBase&&) = default;
  MoveAssignBase& operator=(MoveAssignBase const&) = default;
};

template <typename... Ts>
struct MoveAssignBase<true, Ts...> : CopyAssignBaseAlias<Ts...> {
  using BaseT = CopyAssignBaseAlias<Ts...>;
  using BaseT::BaseT;
};

template <typename... Ts>
using MoveAssignBaseAlias = MoveAssignBase<traits<Ts...>::is_trivial_move_assign, Ts...>;

template <typename... Ts>
struct VariantBase : MoveAssignBaseAlias<Ts...> {
  using BaseT = MoveAssignBaseAlias<Ts...>;

  constexpr VariantBase() noexcept(traits<Ts...>::is_nothrow_default_ctor) : VariantBase(in_place_index<0>) {}

  template <size_t I, typename... Args>
  constexpr explicit VariantBase(in_place_index_t<I> i, Args&&... args) : BaseT(i, std::forward<Args>(args)...) {}

  VariantBase(VariantBase const&) = default;
  VariantBase(VariantBase&&) = default;
  VariantBase& operator=(VariantBase const&) = default;
  VariantBase& operator=(VariantBase&&) = default;
};

template <typename... Ts>
class Variant : private EnableCopyMove<traits<Ts...>::is_copy_ctor, traits<Ts...>::is_copy_assign,
                                       traits<Ts...>::is_move_ctor, traits<Ts...>::is_move_assign, Variant<Ts...>> {};

} // namespace _variant_detail

using _variant_detail::Variant;

} // namespace play
