#pragma once


#include <cstddef>

#include <algorithm>
#include <string_view>
#include <type_traits>


namespace meta {


template <std::size_t N>
struct fstr {
  char str_[N]{};
  consteval /*explicit*/ fstr(char const (&str)[N]) /*noexcept*/ { std::copy_n(str, N, str_); }

  template <std::size_t M>
  consteval auto operator<=>(fstr<M> const& rhs) const /*noexcept*/ {
    return operator std::string_view() <=> rhs;
  }

  template <std::size_t M>
  consteval auto operator==(this auto const& lhs, fstr<M> const& rhs) /*noexcept*/ -> bool {
    return lhs <=> rhs == 0;
  }

  consteval /*explicit*/ operator std::string_view() const /*noexcept*/ { return std::string_view{str_, N - 1}; }
};


namespace detail {

template <fstr... Strs>
consteval auto has_no_duplicate_strs_impl() -> bool {
  constexpr size_t N = sizeof...(Strs);
  if constexpr (N <= 1) {
    return true;
  }
  std::array<std::string_view, N> views{static_cast<std::string_view>(Strs)...};
  std::sort(views.begin(), views.end());
  return std::adjacent_find(views.begin(), views.end()) == views.end();
}

} // namespace detail

template <fstr... Strs>
inline constexpr bool has_no_duplicate_strs_v = detail::has_no_duplicate_strs_impl<Strs...>();

template <fstr... Strs>
struct has_no_duplicate_strs : std::bool_constant<has_no_duplicate_strs_v<Strs...>> {};


} // namespace meta
