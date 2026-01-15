#pragma once


#include "detail/pack/at.hpp"

#include <cstddef>

#include <type_traits>
#include <utility>


namespace meta::pack {


inline namespace lazy {

template <std::size_t I, typename... Ts>
using at = detail::at<I, Ts...>;

template <std::size_t I, typename... Ts>
using at_t = at<I, Ts...>::type;

} // namespace lazy


namespace strict {

template <std::size_t I, typename... Ts>
using at = detail::at<I, Ts...>;

template <std::size_t I, typename... Ts>
using at_t = at<I, Ts...>::type;

namespace value {

using detail::at;

}

} // namespace strict


template <typename T>
using box = std::type_identity<T>;

template <std::size_t I, typename T>
struct indexed_box : box<T> {};

template <typename Indices, typename... Ts>
struct inherit_all_impl;

template <std::size_t... Is, typename... Ts>
struct inherit_all_impl<std::index_sequence<Is...>, Ts...> : indexed_box<Is, Ts>... {};

template <typename... Ts>
using inherit_all = inherit_all_impl<std::make_index_sequence<sizeof...(Ts)>, Ts...>;

// The fold expression here is evaluated eagerly and does not short-circuit.
// Short-circuiting would only be meaningful if there were a large number
// of types with duplicates, which is expected to be rare.
template <typename... Ts>
inline constexpr bool has_no_duplicates_v =
    // NOTE: Fold expressions with `&&` evaluate to `true` for an empty pack.
    (std::is_convertible_v<inherit_all<Ts...>, box<Ts>> && ...);

template <typename... Ts>
struct has_no_duplicates : std::bool_constant<has_no_duplicates_v<Ts...>> {};


} // namespace meta::pack
