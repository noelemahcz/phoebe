#pragma once


#include "../../../config.h"

#include <cstddef>

#include <type_traits>
#include <utility>


namespace meta::pack {


inline namespace lazy {
namespace detail {

#if defined(PHOEBE_IS_CLANG) || defined(PHOEBE_IS_CLANG_CL)

template <std::size_t I, typename... Ts>
struct at {};

template <std::size_t I, typename... Ts>
  requires requires { typename __type_pack_element<I, Ts...>; }
struct at<I, Ts...> {
  using type = __type_pack_element<I, Ts...>;
};

#elif defined(PHOEBE_IS_GCC)

// NOTE:
// GCC's `__type_pack_element` cannot be used in a "requires expression",
// so we fall back to the traditional `void_t` idiom.

template <typename Void, std::size_t I, typename... Ts>
struct at_impl_helper {};

template <std::size_t I, typename... Ts>
struct at_impl_helper<std::void_t<__type_pack_element<I, Ts...>>, I, Ts...> {
  using type = __type_pack_element<I, Ts...>;
};

template <std::size_t I, typename... Ts>
using at_impl = at_impl_helper<void, I, Ts...>;

#else

template <std::size_t I, typename... Ts>
struct at {};

template <typename T0, typename... Tail>
struct at<0, T0, Tail...> {
  using type = T0;
};

template <typename T0, typename T1, typename... Tail>
struct at<1, T0, T1, Tail...> {
  using type = T1;
};

template <typename T0, typename T1, typename T2, typename... Tail>
struct at<2, T0, T1, T2, Tail...> {
  using type = T2;
};

template <typename T0, typename T1, typename T2, typename T3, typename... Tail>
struct at<3, T0, T1, T2, T3, Tail...> {
  using type = T3;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename... Tail>
struct at<4, T0, T1, T2, T3, T4, Tail...> {
  using type = T4;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename... Tail>
struct at<5, T0, T1, T2, T3, T4, T5, Tail...> {
  using type = T5;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename... Tail>
struct at<6, T0, T1, T2, T3, T4, T5, T6, Tail...> {
  using type = T6;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename... Tail>
struct at<7, T0, T1, T2, T3, T4, T5, T6, T7, Tail...> {
  using type = T7;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename... Tail>
struct at<8, T0, T1, T2, T3, T4, T5, T6, T7, T8, Tail...> {
  using type = T8;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename... Tail>
struct at<9, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Tail...> {
  using type = T9;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename... Tail>
struct at<10, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, Tail...> {
  using type = T10;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename... Tail>
struct at<11, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, Tail...> {
  using type = T11;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename... Tail>
struct at<12, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, Tail...> {
  using type = T12;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename... Tail>
struct at<13, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, Tail...> {
  using type = T13;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14,
          typename... Tail>
struct at<14, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, Tail...> {
  using type = T14;
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
          typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14, typename T15,
          typename... Tail>
struct at<15, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, Tail...> {
  using type = T15;
};

template <std::size_t I, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
          typename T7, typename T8, typename T9, typename T10, typename T11, typename T12, typename T13, typename T14,
          typename T15, typename... Tail>
  requires (I >= 16)
struct at<I, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, Tail...> : at<I - 16, Tail...> {};

#endif

} // namespace detail
} // namespace lazy


// FIXME: Move to other place.

template <typename Indices, typename T, template <typename...> typename F>
struct replicate_with {};

template <std::size_t I, typename T>
using expansion_helper = T;

template <std::size_t... Is, typename T, template <typename...> typename F>
struct replicate_with<std::index_sequence<Is...>, T, F> {
  using type = F<expansion_helper<Is, T>...>;
};

template <typename Indices, typename T, template <typename...> typename F>
using replicate_with_t = replicate_with<Indices, T, F>::type;

struct any_type {
  template <typename T>
  constexpr /*explicit*/ any_type(T&&) noexcept {}
};

template <typename... Dummies>
struct matcher_impl {
  template <typename T, typename... Tail>
  static CONSTEVAL auto&& match(Dummies..., T&& value, Tail&&...) noexcept {
    return std::forward<T>(value);
  }
};

template <std::size_t I>
using matcher = replicate_with_t<std::make_index_sequence<I>, any_type, matcher_impl>;

// FIXME: Move to other place.


namespace strict::detail {

template <std::size_t I, typename... Ts>
struct at {
  using type = std::remove_cvref_t<decltype(matcher<I>::match(std::type_identity<Ts>{}...))>::type;
};

} // namespace strict::detail


namespace value::strict::detail {

// NOTE:
// This function relies on P2280 ("Using unknown pointers and references in constant expressions").
// Even when called with arguments referring to runtime values, these "unknown references" are valid
// in a constant expression provided no lvalue-to-rvalue conversion (value reading) occurs.
template <std::size_t I, typename... Ts>
CONSTEVAL auto&& at(Ts&&... values) noexcept {
  return matcher<I>::match(std::forward<Ts>(values)...);
}

} // namespace value::strict::detail


} // namespace meta::pack
