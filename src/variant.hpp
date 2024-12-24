#pragma once
#include <functional>
#include <utility>

#include "enable_smfs.hpp"
#include "trait.hpp"

namespace play {
template <typename... Ts>
class Variant;

inline constexpr size_t variant_npos = -1;

// TRICK
// 提取基础类型（不含 cv/ref 限定）的小技巧：在目标类型之后设置一个默认值为 rm_cv_ref<T> 的辅助类型
// 特化时专门用基础类型特化这个辅助类型，这样目标类型数就能自动匹配 cv/ref 之间的组合
template <typename V, typename = std::remove_cv_t<std::remove_reference_t<V>>>
struct variant_size;

template <typename V, typename... Ts>
struct variant_size<V, Variant<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

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

template <size_t I, typename Union>
constexpr decltype(auto) getN(Union&& u) noexcept {
  if constexpr (I == 0) {
    return std::forward<Union>(u).m_first.get();
  }
  if constexpr (I == 1) {
    return std::forward<Union>(u).m_rest.m_first.get();
  }
  if constexpr (I == 2) {
    return std::forward<Union>(u).m_rest.m_rest.m_first.get();
  } else {
    return getN<I - 3>(std::forward<Union>(u).m_rest.m_rest.m_rest);
  }
}

template <size_t I, typename V>
constexpr decltype(auto) get(V&& var) noexcept {
  return getN<I>(std::forward<V>(var).m_union);
}

// Suitably-small, trivially copyable types can always be placed in a variant without it becoming valueless.
template <typename T>
using never_valueless_alt = std::conjunction<std::bool_constant<sizeof(T) <= 256>, std::is_trivially_copyable<T>>;

template <typename... Ts>
using never_valueless = std::conjunction<std::bool_constant<traits<Ts...>::is_move_assign>, never_valueless_alt<Ts>...>;

template <typename Maybe_variant_cooke, typename V, typename = std::remove_cv_t<std::remove_reference_t<V>>>
inline constexpr bool extra_visit_slot_needed = false;

template <typename V, typename... Ts>
inline constexpr bool extra_visit_slot_needed<variant_cookie, V, Variant<Ts...>> = !never_valueless<Ts...>::value;

template <typename V, typename... Ts>
inline constexpr bool extra_visit_slot_needed<variant_idx_cookie, V, Variant<Ts...>> = !never_valueless<Ts...>::value;

// Param dimensions are the number of alternatives of each variant in sequence.
template <typename Lambda, size_t... dimensions>
struct MultiArray;

// Base case.
// The only one data member m_data store the address of a function.
template <typename Lambda>
struct MultiArray<Lambda> {
  template <typename>
  struct untag_result : std::false_type {
    using type = Lambda;
  };

  template <typename... Args>
  struct untag_result<const void (*)(Args...)> : std::false_type {
    using type = void (*)(Args...);
  };

  template <typename... Args>
  struct untag_result<variant_cookie (*)(Args...)> : std::false_type {
    using type = void (*)(Args...);
  };

  template <typename... Args>
  struct untag_result<variant_idx_cookie (*)(Args...)> : std::false_type {
    using type = void (*)(Args...);
  };

  template <typename Ret, typename... Args>
  struct untag_result<deduce_visit_result<Ret> (*)(Args...)> : std::true_type {
    using type = Ret (*)(Args...);
  };

  using ElementType = typename untag_result<Lambda>::type;

  using result_is_deduced = untag_result<Lambda>;

  constexpr const ElementType& access() const { return m_data; }

  ElementType m_data;
};

// Recursion case.
template <typename Ret, typename Visitor, typename... Vs, size_t first, size_t... rest>
struct MultiArray<Ret (*)(Visitor, Vs...), first, rest...> {
  using V = nth_type_t<sizeof...(Vs) - sizeof...(rest) - 1, Vs...>;
  using Lambda = Ret (*)(Visitor, Vs...);

  static constexpr size_t do_cookie = extra_visit_slot_needed<Ret, V> ? 1 : 0;

  template <typename... Args>
  constexpr decltype(auto) access(size_t first_index, Args... rest_indices) const {
    return m_arr[first_index + do_cookie].access(rest_indices...);
  }

  MultiArray<Lambda, rest...> m_arr[first + do_cookie];
};

template <typename ArrayType, typename IdxSeq>
struct gen_vtable_impl;

// Base case.
// It populates a MultiArray element with the address of a function
// that invokes the visitor with the alternatives specified by indices.
template <typename Ret, typename Visitor, typename... Vs, size_t... indices>
struct gen_vtable_impl<MultiArray<Ret (*)(Visitor, Vs...)>, std::index_sequence<indices...>> {
  using ArrayType = MultiArray<Ret (*)(Visitor, Vs...)>;

  template <size_t index, typename V>
  static constexpr decltype(auto) elementByIndexOrCookie(V&& var) noexcept {
    if constexpr (index != variant_npos) {
      return get<index>(std::forward<V>(var));
    } else {
      return variant_cookie{};
    }
  }

  static constexpr decltype(auto) visitInvoke(Visitor&& visitor, Vs... vars) {
    if constexpr (std::is_same_v<Ret, variant_idx_cookie>) {
      // For raw visitation using indices, pass the indices to the visitor and discard the return value
      std::invoke(std::forward<Visitor>(visitor), elementByIndexOrCookie<indices>(std::forward<Vs>(vars))...,
                  std::integral_constant<size_t, indices>()...);
    } else if constexpr (std::is_same_v<Ret, variant_cookie>) {
      // For raw visitation without indices, and discard the return value
      std::invoke(std::forward<Visitor>(visitor), elementByIndexOrCookie<indices>(std::forward<Vs>(vars))...);
    } else if constexpr (ArrayType::result_is_deduced::value) {
      // For usual case
      return std::invoke(std::forward<Visitor>(visitor), elementByIndexOrCookie<indices>(std::forward<Vs>(vars))...);
    } else {
      // For visit<R>
      return std::invoke(std::forward<Visitor>(visitor), get<indices>(std::forward<Vs>(vars))...);
    }
  }

  static constexpr auto apply() {
    if constexpr (ArrayType::result_is_deduced::value) {
      constexpr bool visit_ret_type_mismatch =
          !std::is_same_v<typename Ret::type, decltype(visitInvoke(std::declval<Visitor>(), std::declval<Vs>()...))>;
      if constexpr (visit_ret_type_mismatch) {
        struct cannot_match {};
        return cannot_match{};
      } else {
        return ArrayType{&visitInvoke};
      }
    } else {
      return ArrayType{&visitInvoke};
    }
  }
};

// Recursion case.
// It builds up the index sequences by recursively
// calling apply() on the next specialization of gen_vtable_impl.
// The base case of the recursion defines the actual function pointers.
template <typename Ret, typename Visitor, typename... Vs, size_t... dimensions, size_t... indices>
struct gen_vtable_impl<MultiArray<Ret (*)(Visitor, Vs...), dimensions...>, std::index_sequence<indices...>> {
  using ArrayType = MultiArray<Ret (*)(Visitor, Vs...), dimensions...>;
  using NextV = std::remove_reference<nth_type_t<sizeof...(indices), Vs...>>;

  template <bool do_cookie, size_t index, typename T>
  static constexpr void applySingleAlt(T& element, T* cookie_element = nullptr) {
    if constexpr (do_cookie) {
      element = gen_vtable_impl<T, std::index_sequence<indices..., index>>::apply();
      *cookie_element = gen_vtable_impl<T, std::index_sequence<indices..., variant_npos>>::apply();
    } else {
      auto tmp_element =
          gen_vtable_impl<std::remove_reference_t<decltype(element)>, std::index_sequence<indices..., index>>::apply();
      static_assert(std::is_same_v<T, decltype(tmp_element)>,
                    "std::visit requires the visitor to have the same "
                    "return type for all alternatives of a variant");
      element = tmp_element;
    }
  }

  template <size_t... v_indices>
  static constexpr void applyAllAlts(ArrayType& vtable, std::index_sequence<v_indices...>) {
    if constexpr (extra_visit_slot_needed<Ret, NextV>) {
      (applySingleAlt<true, v_indices>(vtable.m_arr[v_indices + 1], &(vtable.m_arr[0])), ...);
    } else {
      (applySingleAlt<false, v_indices>(vtable.m_arr[v_indices]), ...);
    }
  }

  static constexpr ArrayType apply() {
    ArrayType vtable{};
    applyAllAlts(vtable, std::make_index_sequence<variant_size_v<NextV>>());
    return vtable;
  }
};

template <typename Ret, typename Visitor, typename... Vs>
struct gen_vtable {
  using ArrayType = MultiArray<Ret (*)(Visitor, Vs...), variant_size_v<Vs>...>;

  static constexpr ArrayType vtable = gen_vtable_impl<ArrayType, std::index_sequence<>>::apply();
};

template <typename Ret, typename Visitor, typename... Vs>
constexpr decltype(auto) doVisit(Visitor&& visitor, Vs&&... vars) {
  // early return for case of visiting no variants
  if constexpr (sizeof...(Vs) == 0) {
    if constexpr (std::is_void_v<Ret>) {
      return (void)std::forward<Visitor>(visitor)();
    } else {
      return std::forward<Visitor>(visitor)();
    }
  } else {
    using V0 = nth_type_t<0, Vs...>;
    using ArrayType = MultiArray<Ret (*)(Visitor&&, V0&&)>;

    constexpr size_t v0_size = variant_size_v<V0>;

    if constexpr (sizeof...(Vs) == 1 && v0_size <= 11) {
      V0& v0 = [](V0& v, ...) -> V0& { return v; }(vars...);

#define VISIT_UNREACHABLE __builtin_unreachable
#define VISIT_CASE(N)                                                                                        \
  case N: {                                                                                                  \
    if constexpr (N < v0_size) {                                                                             \
      return gen_vtable_impl<ArrayType, std::index_sequence<N>>::visitInvoke(std::forward<Visitor>(visitor), \
                                                                             std::forward<V0>(v0));          \
    } else {                                                                                                 \
      VISIT_UNREACHABLE();                                                                                   \
    }                                                                                                        \
  }

      switch (v0.index()) {
        VISIT_CASE(0)
        VISIT_CASE(1)
        VISIT_CASE(2)
        VISIT_CASE(3)
        VISIT_CASE(4)
        VISIT_CASE(5)
        VISIT_CASE(6)
        VISIT_CASE(7)
        VISIT_CASE(8)
        VISIT_CASE(9)
        VISIT_CASE(10)
        case variant_npos:
          if constexpr (std::is_same_v<Ret, variant_cookie> || std::is_same_v<Ret, variant_idx_cookie>) {
            return gen_vtable_impl<ArrayType, std::index_sequence<variant_npos>>::visitInvoke(
                std::forward<Visitor>(visitor), std::forward<V0>(v0));
          } else {
            VISIT_UNREACHABLE();
          }
        default:
          VISIT_UNREACHABLE();
      }
#undef VISIT_UNREACHABLE
#undef VISIT_CASE
    } else {
      // too many variants or too many alternatives of the first variant, using a jump table to generate case
      constexpr auto& vtable = gen_vtable<Ret, Visitor&&, Vs&&...>::vtable;
      auto func_ptr = vtable.access(vars.index()...);
      return (*func_ptr)(std::forward<Visitor>(visitor), std::forward<Vs>(vars)...);
    }
  }
}

template <typename T, bool = std::is_trivially_destructible_v<T>>
struct Uninitialized;

template <typename T>
struct Uninitialized<T, true> {
  template <typename... Args>
  constexpr Uninitialized(in_place_index_t<0>, Args&&... args) : m_storage(std::forward<Args>(args)...) {}

  constexpr const T& get() const& noexcept { return m_storage; }
  constexpr T& get() & noexcept { return m_storage; }
  constexpr const T&& get() const&& noexcept { return std::move(m_storage); }
  constexpr T&& get() && noexcept { return std::move(m_storage); }

  T m_storage;
};

template <typename T>
struct Uninitialized<T, false> {
  template <typename... Args>
  constexpr Uninitialized(in_place_index_t<0>, Args&&... args) {
    new ((void*)std::addressof(m_storage)) T(std::forward<Args>(args)...);
  }

  const T& get() const& noexcept { return *reinterpret_cast<const T*>(&m_storage); }
  T& get() & noexcept { return *reinterpret_cast<T*>(&m_storage); }
  const T&& get() const&& noexcept { return std::move(*reinterpret_cast<const T*>(&m_storage)); }
  T&& get() && noexcept { return std::move(*reinterpret_cast<T*>(&m_storage)); }

  alignas(alignof(T)) unsigned char m_storage[sizeof(T)];
};

// Inheritance chain:
// VariantStorage(VariantUnion) -> CopyCtorBase -> MoveCtorBase -> CopyAssignBase -> MoveAssignBase -> Variant

// To get the value in union, use get<I>(u)
template <bool trivially_dtor, typename... Ts>
union VariadicUnion {
  VariadicUnion() = default;

  template <size_t I, typename... Args>
  VariadicUnion(in_place_index_t<I>, Args&&...) = delete;
};

template <bool trivially_dtor, typename First, typename... Rest>
union VariadicUnion<trivially_dtor, First, Rest...> {
  constexpr VariadicUnion() : m_rest() {}

  template <typename... Args>
  constexpr VariadicUnion(in_place_index_t<0>, Args&&... args)
      : m_first(in_place_index<0>, std::forward<Args>(args)...) {}

  template <size_t I, typename... Args>
  constexpr VariadicUnion(in_place_index_t<I>, Args&&... args)
      : m_rest(in_place_index<I - 1>, std::forward<Args>(args)...) {}

  Uninitialized<First> m_first;
  VariadicUnion<trivially_dtor, Rest...> m_rest;
};

template <bool trivially_dtor, typename... Ts>
struct VariantStorage {
  // In fact using a smaller unsigned integer type instead of size_t could save more space.
  // In std, they choose to use unsigned char/short base on the size of the parameter pack.
  using IndexType = size_t;
  // TODO
};

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

  CopyCtorBase(const CopyCtorBase&) noexcept(traits<Ts...>::is_nothrow_copy_ctor) {
    // TODO
  };

  CopyCtorBase(CopyCtorBase&&) = default;
  CopyCtorBase& operator=(const CopyCtorBase&) = default;
  CopyCtorBase& operator=(CopyCtorBase&&) = default;
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

  MoveCtorBase(const MoveCtorBase&) = default;
  MoveCtorBase& operator=(const MoveCtorBase&) = default;
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

  // If not all alternatives are trivially copy assignable, need to manually invoke copy opeartion.
  // Other smfs need to be handled in similar way
  CopyAssignBase& operator=(const CopyAssignBase&) noexcept(traits<Ts...>::is_nothrow_copy_assign) {
    // TODO
  };

  CopyAssignBase(const CopyAssignBase&) = default;
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

  MoveAssignBase(const MoveAssignBase&) = default;
  MoveAssignBase(MoveAssignBase&&) = default;
  MoveAssignBase& operator=(const MoveAssignBase&) = default;
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

  VariantBase(const VariantBase&) = default;
  VariantBase(VariantBase&&) = default;
  VariantBase& operator=(const VariantBase&) = default;
  VariantBase& operator=(VariantBase&&) = default;
};

} // namespace _variant_detail

template <typename... Ts>
class Variant : private EnableCopyMove<_variant_detail::traits<Ts...>::is_copy_ctor,
                                       _variant_detail::traits<Ts...>::is_copy_assign,
                                       _variant_detail::traits<Ts...>::is_move_ctor,
                                       _variant_detail::traits<Ts...>::is_move_assign, Variant<Ts...>> {
private:
  template <size_t I, typename V>
  friend constexpr decltype(auto) _variant_detail::get(V&& var) noexcept;
};

} // namespace play
