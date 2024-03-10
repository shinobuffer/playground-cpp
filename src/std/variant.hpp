#pragma once

#include <functional>
#include <utility>

#include "enable_smfs.hpp"
#include "memory.hpp"
#include "trait.hpp"

namespace play {
template <typename... Ts>
class Variant;

inline constexpr size_t variant_npos = -1;

// TRICK
// 提取基础类型（不含 cv/ref 限定）的小技巧：在目标类型之后设置一个默认值为 rm_cv_ref<T> 的辅助类型
// 特化时专门用基础类型特化这个辅助类型，这样目标类型数就能自动匹配 cv/ref 之间的组合
template <typename V, typename = remove_cvref_t<V>>
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
// Tag for raw visit
struct variant_cookie {};
// Tag for raw visit with with indices passed in
struct variant_idx_cookie {
  using type = variant_idx_cookie;
};
// Tag for enable deduction (and same-type checking) for visit
template <typename T>
struct deduce_visit_result {
  using type = T;
};

template <typename... Ts>
struct traits {
  static constexpr bool s_default_ctor = std::is_default_constructible_v<nth_type<0, Ts...>>;
  static constexpr bool s_copy_ctor = (std::is_copy_constructible_v<Ts> && ...);
  static constexpr bool s_move_ctor = (std::is_move_constructible_v<Ts> && ...);
  static constexpr bool s_copy_assign = s_copy_ctor && (std::is_copy_assignable_v<Ts> && ...);
  static constexpr bool s_move_assign = s_move_ctor && (std::is_move_assignable_v<Ts> && ...);

  static constexpr bool s_trivial_dtor = (std::is_trivially_destructible_v<Ts> && ...);
  static constexpr bool s_trivial_copy_ctor = (std::is_trivially_copy_constructible_v<Ts> && ...);
  static constexpr bool s_trivial_move_ctor = (std::is_trivially_move_constructible_v<Ts> && ...);
  static constexpr bool s_trivial_copy_assign = s_trivial_copy_ctor && (std::is_trivially_copy_assignable_v<Ts> && ...);
  static constexpr bool s_trivial_move_assign = s_trivial_move_ctor && (std::is_trivially_move_assignable_v<Ts> && ...);

  static constexpr bool s_nothrow_default_ctor = std::is_nothrow_default_constructible_v<nth_type_t<0, Ts...>>;
  static constexpr bool s_nothrow_copy_ctor = false;
  static constexpr bool s_nothrow_move_ctor = (std::is_nothrow_move_constructible_v<Ts> && ...);
  static constexpr bool s_nothrow_copy_assign = false;
  static constexpr bool s_nothrow_move_assign = s_nothrow_move_ctor && (std::is_nothrow_move_assignable_v<Ts> && ...);
};

// Suitably-small, trivially copyable type can always be placed in a variant without it becoming valueless.
template <typename T>
using never_valueless_alt = std::conjunction<std::bool_constant<sizeof(T) <= 256>, std::is_trivially_copyable<T>>;

// True for Variant<Ts...> that never be valueless
template <typename... Ts>
using never_valueless = std::conjunction<std::bool_constant<traits<Ts...>::s__move_assign>, never_valueless_alt<Ts>...>;

template <typename MaybeVariantCookie, typename V, typename = remove_cvref_t<V>>
inline constexpr bool extra_visit_slot_needed = false;

template <typename V, typename... Ts>
inline constexpr bool extra_visit_slot_needed<variant_cookie, V, Variant<Ts...>> = !never_valueless<Ts...>::value;

template <typename V, typename... Ts>
inline constexpr bool extra_visit_slot_needed<variant_idx_cookie, V, Variant<Ts...>> = !never_valueless<Ts...>::value;

template <typename... Ts, typename T>
decltype(auto) variant_cast(T&& rhs) {
  if constexpr (std::is_lvalue_reference_v<T>) {
    if constexpr (std::is_const_v<std::remove_reference<T>>) {
      return static_cast<const Variant<Ts...>&>(rhs);
    } else {
      return static_cast<Variant<Ts...>&>(rhs);
    }
  } else {
    return static_cast<Variant<Ts...>&&>(rhs);
  }
}

template <size_t I, typename Union>
constexpr decltype(auto) getN(Union&& u) noexcept {
  if constexpr (I == 0) {
    return std::forward<Union>(u).m_first.get();
  } else if constexpr (I == 1) {
    return std::forward<Union>(u).m_rest.m_first.get();
  } else if constexpr (I == 2) {
    return std::forward<Union>(u).m_rest.m_rest.m_first.get();
  } else {
    return getN<I - 3>(std::forward<Union>(u).m_rest.m_rest.m_rest);
  }
}

template <size_t I, typename V>
constexpr decltype(auto) get(V&& var) noexcept {
  return getN<I>(std::forward<V>(var).m_union);
}

template <size_t I, typename Union>
constexpr decltype(auto) constructN(Union& u) noexcept {
  if constexpr (I == 0) {
    return &u.m_first;
  } else if constexpr (I == 1) {
    construct_at(&u.m_rest);
    return &u.m_rest.m_first;
  } else if constexpr (I == 2) {
    construct_at(&u.m_rest);
    construct_at(&u.m_rest.m_rest);
    return &u.m_rest.m_rest.m_first;
  } else {
    construct_at(&u.m_rest);
    construct_at(&u.m_rest.m_rest);
    construct_at(&u.m_rest.m_rest.m_rest);
    return constructN<I - 3>(u.m_rest.m_rest.m_rest);
  }
}

// @dimensions are the number of alternatives of each variant in sequence.
template <typename Lambda, size_t... dimensions>
struct MultiArray;

// Base case.
// With only one data member m_data store the address of a function.
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
  static constexpr decltype(auto) s_elementByIndexOrCookie(V&& var) noexcept {
    if constexpr (index != variant_npos) {
      return get<index>(std::forward<V>(var));
    } else {
      return variant_cookie{};
    }
  }

  static constexpr decltype(auto) s_visitInvoke(Visitor&& visitor, Vs... vars) {
    if constexpr (std::is_same_v<Ret, variant_idx_cookie>) {
      // For raw visitation using indices, pass the indices to the visitor and discard the return value
      std::invoke(std::forward<Visitor>(visitor), s_elementByIndexOrCookie<indices>(std::forward<Vs>(vars))...,
                  std::integral_constant<size_t, indices>()...);
    } else if constexpr (std::is_same_v<Ret, variant_cookie>) {
      // For raw visitation without indices, and discard the return value
      std::invoke(std::forward<Visitor>(visitor), s_elementByIndexOrCookie<indices>(std::forward<Vs>(vars))...);
    } else if constexpr (ArrayType::result_is_deduced::value) {
      // For usual case
      return std::invoke(std::forward<Visitor>(visitor), s_elementByIndexOrCookie<indices>(std::forward<Vs>(vars))...);
    } else {
      // For visit<R>
      return std::invoke(std::forward<Visitor>(visitor), get<indices>(std::forward<Vs>(vars))...);
    }
  }

  static constexpr auto s_apply() {
    if constexpr (ArrayType::result_is_deduced::value) {
      constexpr bool visit_ret_type_mismatch =
          !std::is_same_v<typename Ret::type, decltype(s_visitInvoke(std::declval<Visitor>(), std::declval<Vs>()...))>;
      if constexpr (visit_ret_type_mismatch) {
        struct cannot_match {};
        return cannot_match{};
      } else {
        return ArrayType{&s_visitInvoke};
      }
    } else {
      return ArrayType{&s_visitInvoke};
    }
  }
};

// Recursion case.
// It builds up the index sequences by recursively
// calling s_apply() on the next specialization of gen_vtable_impl.
// The base case of the recursion defines the actual function pointers.
template <typename Ret, typename Visitor, typename... Vs, size_t... dimensions, size_t... indices>
struct gen_vtable_impl<MultiArray<Ret (*)(Visitor, Vs...), dimensions...>, std::index_sequence<indices...>> {
  using ArrayType = MultiArray<Ret (*)(Visitor, Vs...), dimensions...>;
  using NextV = std::remove_reference<nth_type_t<sizeof...(indices), Vs...>>;

  template <bool do_cookie, size_t index, typename T>
  static constexpr void s_applySingleAlt(T& element, T* cookie_element = nullptr) {
    if constexpr (do_cookie) {
      element = gen_vtable_impl<T, std::index_sequence<indices..., index>>::s_apply();
      *cookie_element = gen_vtable_impl<T, std::index_sequence<indices..., variant_npos>>::s_apply();
    } else {
      auto tmp_element = gen_vtable_impl<std::remove_reference_t<decltype(element)>,
                                         std::index_sequence<indices..., index>>::s_apply();
      static_assert(std::is_same_v<T, decltype(tmp_element)>,
                    "play::visit requires the visitor to have the same "
                    "return type for all alternatives of a variant");
      element = tmp_element;
    }
  }

  template <size_t... v_indices>
  static constexpr void s_applyAllAlts(ArrayType& vtable, std::index_sequence<v_indices...>) {
    if constexpr (extra_visit_slot_needed<Ret, NextV>) {
      (s_applySingleAlt<true, v_indices>(vtable.m_arr[v_indices + 1], &(vtable.m_arr[0])), ...);
    } else {
      (s_applySingleAlt<false, v_indices>(vtable.m_arr[v_indices]), ...);
    }
  }

  static constexpr ArrayType s_apply() {
    ArrayType vtable{};
    s_applyAllAlts(vtable, std::make_index_sequence<variant_size_v<NextV>>());
    return vtable;
  }
};

template <typename Ret, typename Visitor, typename... Vs>
struct gen_vtable {
  using ArrayType = MultiArray<Ret (*)(Visitor, Vs...), variant_size_v<Vs>...>;

  static constexpr ArrayType s_vtable = gen_vtable_impl<ArrayType, std::index_sequence<>>::s_apply();
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
    constexpr size_t v0_size = variant_size_v<V0>;

    if constexpr (sizeof...(Vs) == 1 && v0_size <= 11) {
      using ArrayType = MultiArray<Ret (*)(Visitor&&, V0&&)>;
      V0& v0 = [](V0& v, ...) -> V0& { return v; }(vars...);

#define VISIT_UNREACHABLE __builtin_unreachable
#define VISIT_CASE(N)                                                                                          \
  case N: {                                                                                                    \
    if constexpr (N < v0_size) {                                                                               \
      return gen_vtable_impl<ArrayType, std::index_sequence<N>>::s_visitInvoke(std::forward<Visitor>(visitor), \
                                                                               std::forward<V0>(v0));          \
    } else {                                                                                                   \
      VISIT_UNREACHABLE();                                                                                     \
    }                                                                                                          \
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
            return gen_vtable_impl<ArrayType, std::index_sequence<variant_npos>>::s_visitInvoke(
                std::forward<Visitor>(visitor), std::forward<V0>(v0));
          } else {
            VISIT_UNREACHABLE();
          }
        default:
          VISIT_UNREACHABLE();
      }
#undef VISIT_CASE
#undef VISIT_UNREACHABLE
    } else {
      // too many variants or too many alternatives of the first variant, using a jump table to generate case
      constexpr auto& vtable = gen_vtable<Ret, Visitor&&, Vs&&...>::s_vtable;
      auto func_ptr = vtable.access(vars.index()...);
      return (*func_ptr)(std::forward<Visitor>(visitor), std::forward<Vs>(vars)...);
    }
  }
}

template <typename Visitor, typename... Vs>
constexpr void rawVisit(Visitor&& visitor, Vs&&... vars) {
  doVisit<variant_cookie>(std::forward<Visitor>(visitor), std::forward<Vs>(vars)...);
}

template <typename Visitor, typename... Vs>
constexpr void rawIdxVisit(Visitor&& visitor, Vs&&... vars) {
  doVisit<variant_idx_cookie>(std::forward<Visitor>(visitor), std::forward<Vs>(vars)...);
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
  // Tip: using a smaller unsigned integer type base on the number of alternatives could save more space
  using IndexType = size_t;
};

template <typename... Ts>
struct VariantStorage<false, Ts...> {
  using IndexType = size_t;

  constexpr VariantStorage() : m_index(static_cast<IndexType>(variant_npos)) {}

  template <size_t I, typename... Args>
  constexpr VariantStorage(in_place_index_t<I>, Args&&... args)
      : m_union(in_place_index<I>, std::forward<Args>(args)...), m_index(I) {}

  ~VariantStorage() { reset(); }

  constexpr void reset() {
    if (!valid()) [[__unlikely__]] {
      return;
    }
    doVisit<void>([](auto&& self) mutable { std::destroy_at(std::addressof(self)); }, variant_cast<Ts...>(*this));
    m_index = static_cast<IndexType>(variant_npos);
  }

  constexpr bool valid() const noexcept {
    if constexpr (never_valueless<Ts...>::value) {
      return true;
    } else {
      return m_index != IndexType(variant_npos);
    }
  }

  VariadicUnion<false, Ts...> m_union;
  IndexType m_index;
};

template <typename... Ts>
struct VariantStorage<true, Ts...> {
  using IndexType = size_t;

  constexpr VariantStorage() : m_index(static_cast<IndexType>(variant_npos)) {}

  template <size_t I, typename... Args>
  constexpr VariantStorage(in_place_index_t<I>, Args&&... args)
      : m_union(in_place_index<I>, std::forward<Args>(args)...), m_index(I) {}

  constexpr void reset() noexcept { m_index = static_cast<IndexType>(variant_npos); }

  constexpr bool valid() const noexcept {
    if constexpr (never_valueless<Ts...>::value) {
      return true;
    } else {
      return m_index != IndexType(variant_npos);
    }
  }

  VariadicUnion<true, Ts...> m_union;
  IndexType m_index;
};

template <size_t I, bool trivially_dtor, typename... Ts, typename... Args>
inline void emplace(VariantStorage<trivially_dtor, Ts...>& storage, Args&&... args) {
  storage.reset();
  construct_at(constructN<I>(storage.m_union), in_place_index<0>, std::forward<Args>(args)...);
  storage.m_index = I;
}

template <typename... Ts>
using VariantStorageAlias = VariantStorage<traits<Ts...>::s_trivial_dtor, Ts...>;

template <bool trivially_copy_ctor, typename... Ts>
struct CopyCtorBase : VariantStorageAlias<Ts...> {
  using BaseT = VariantStorageAlias<Ts...>;
  using BaseT::BaseT;

  CopyCtorBase(const CopyCtorBase& rhs) noexcept(traits<Ts...>::s_nothrow_copy_ctor) {
    rawIdxVisit(
        [this](auto&& rhs_value, auto rhs_index) mutable {
          constexpr size_t I = rhs_index;
          if constexpr (I != variant_npos) {
            construct_at(&this->m_union, in_place_index<I>, rhs_value);
          }
        },
        variant_cast<Ts...>(rhs));
    this->m_index = rhs.m_index;
  };

  CopyCtorBase(CopyCtorBase&&) = default;
  CopyCtorBase& operator=(const CopyCtorBase&) = default;
  CopyCtorBase& operator=(CopyCtorBase&&) = default;
};

template <typename... Ts>
struct CopyCtorBase<true, Ts...> : VariantStorageAlias<Ts...> {
  using BaseT = VariantStorageAlias<Ts...>;
  using BaseT::BaseT;
};

template <typename... Ts>
using CopyCtorBaseAlias = CopyCtorBase<traits<Ts...>::s_trivial_copy_ctor, Ts...>;

template <bool trivially_move_ctor, typename... Ts>
struct MoveCtorBase : CopyCtorBaseAlias<Ts...> {
  using BaseT = CopyCtorBaseAlias<Ts...>;
  using BaseT::BaseT;

  MoveCtorBase(MoveCtorBase&& rhs) noexcept(traits<Ts...>::s_nothrow_move_ctor) {
    rawIdxVisit(
        [this](auto&& rhs_value, auto rhs_index) mutable {
          constexpr size_t I = rhs_index;
          if constexpr (I != variant_npos) {
            construct_at(&this->m_union, in_place_index<I>, std::forward<decltype(rhs_value)>(rhs_value));
          }
        },
        variant_cast<Ts...>(std::move(rhs)));
    this->m_index = rhs.m_index;
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
using MoveCtorBaseAlias = MoveCtorBase<traits<Ts...>::s_trivial_move_ctor, Ts...>;

template <bool trivially_copy_assign, typename... Ts>
struct CopyAssignBase : MoveCtorBaseAlias<Ts...> {
  using BaseT = MoveCtorBaseAlias<Ts...>;
  using BaseT::BaseT;

  CopyAssignBase& operator=(const CopyAssignBase& rhs) noexcept(traits<Ts...>::s_nothrow_copy_assign) {
    rawIdxVisit(
        [this](auto&& rhs_value, auto rhs_index) mutable {
          constexpr size_t I = rhs_index;
          if constexpr (I == variant_npos) {
            this->reset();
          } else if (this->m_index == I) {
            get<I>(*this) = rhs_value;
          } else {
            using Ti = nth_type_t<I, Ts...>;
            if constexpr (std::is_nothrow_copy_constructible_v<Ti> || !std::is_nothrow_move_constructible_v<Ti>) {
              emplace<I>(*this, rhs_value);
            } else {
              using V = Variant<Ts...>;
              V& self = variant_cast<Ts...>(*this);
              self = V(in_place_index<I>, rhs_value);
            }
          }
        },
        variant_cast<Ts...>(rhs));
    return *this;
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
using CopyAssignBaseAlias = CopyAssignBase<traits<Ts...>::s_trivial_copy_assign, Ts...>;

template <bool trivially_move_assign, typename... Ts>
struct MoveAssignBase : CopyAssignBaseAlias<Ts...> {
  using BaseT = CopyAssignBaseAlias<Ts...>;
  using BaseT::BaseT;

  // If not all alternatives are trivially copy assignable, need to manually invoke copy opeartion.
  // Other smfs need to be handled in similar way
  MoveAssignBase& operator=(MoveAssignBase&& rhs) noexcept(traits<Ts...>::s_nothrow_move_assign) {
    rawIdxVisit(
        [this](auto&& rhs_value, auto rhs_index) mutable {
          constexpr size_t I = rhs_index;
          if constexpr (I == variant_npos) {
            this->reset();
          } else if (this->m_index == I) {
            get<I>(*this) = std::move(rhs_value);
          } else {
            using Ti = nth_type_t<I, Ts...>;
            if constexpr (std::is_nothrow_move_constructible_v<Ti>) {
              emplace<I>(*this, std::move(rhs_value));
            } else {
              using V = Variant<Ts...>;
              V& self = variant_cast<Ts...>(*this);
              self.template emplace<I>(std::move(rhs_value));
            }
          }
        },
        variant_cast<Ts...>(rhs));
    return *this;
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
using MoveAssignBaseAlias = MoveAssignBase<traits<Ts...>::s_trivial_move_assign, Ts...>;

template <typename... Ts>
struct VariantBase : MoveAssignBaseAlias<Ts...> {
  using BaseT = MoveAssignBaseAlias<Ts...>;

  constexpr VariantBase() noexcept(traits<Ts...>::s_nothrow_default_ctor) : VariantBase(in_place_index<0>) {}

  template <size_t I, typename... Args>
  constexpr explicit VariantBase(in_place_index_t<I> i, Args&&... args) : BaseT(i, std::forward<Args>(args)...) {}

  VariantBase(const VariantBase&) = default;
  VariantBase(VariantBase&&) = default;
  VariantBase& operator=(const VariantBase&) = default;
  VariantBase& operator=(VariantBase&&) = default;
};
} // namespace _variant_detail

class BadVariantAccess : public std::exception {
public:
  BadVariantAccess() noexcept {}

  const char* what() const noexcept override { return m_reason; }

private:
  BadVariantAccess(const char* reason) noexcept : m_reason(reason) {}

  const char* m_reason = "bad variant access";
};

template <typename... Ts>
class Variant
    : private EnableCopyMove<_variant_detail::traits<Ts...>::s_copy_ctor, _variant_detail::traits<Ts...>::s_copy_assign,
                             _variant_detail::traits<Ts...>::s_move_ctor, _variant_detail::traits<Ts...>::s_move_assign,
                             Variant<Ts...>> {
private:
  template <size_t I, typename V>
  friend constexpr decltype(auto) _variant_detail::get(V&& var) noexcept;
};
} // namespace play
