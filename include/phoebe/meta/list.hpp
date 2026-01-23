#pragma once


#include "detail/pack/at.hpp"

#include <cstddef>

#include <type_traits>


namespace meta::list {


template <typename... Ts>
struct list;


template <typename List>
inline constexpr bool empty_v = false;

template <>
inline constexpr bool empty_v<list<>> = true;

template <typename List>
struct empty : std::bool_constant<empty_v<List>> {};


template <typename List>
struct length {};

template <typename... Ts>
struct length<list<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <typename List>
inline constexpr std::size_t length_v = length<List>::value;


namespace lazy {

template <std::size_t I, typename List>
struct at {};

template <std::size_t I, typename... Ts>
struct at<I, list<Ts...>> : pack::lazy::detail::at<I, Ts...> {};

template <std::size_t I, typename List>
using at_t = at<I, List>::type;

} // namespace lazy


namespace strict {

template <std::size_t I, typename List>
struct at {};

template <std::size_t I, typename... Ts>
struct at<I, list<Ts...>> : pack::strict::detail::at<I, Ts...> {};

template <std::size_t I, typename List>
using at_t = at<I, List>::type;

} // namespace strict


} // namespace meta::list
