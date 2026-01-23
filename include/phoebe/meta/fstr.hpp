#pragma once


#include <cstddef>

#include <algorithm>
#include <string_view>
#include <type_traits>


namespace meta {


template <std::size_t N>
struct fstr {
  char buf[N]{};

  consteval /*explicit*/ fstr(char const (&str)[N]) noexcept { std::copy_n(str, N, buf); }

  consteval /*explicit*/ operator std::string_view() const noexcept { return std::string_view{buf, N - 1}; }

  template <std::size_t M>
  consteval auto operator<=>(fstr<M> const& rhs) const noexcept {
    return operator std::string_view() <=> rhs;
  }

  template <std::size_t M>
  consteval auto operator==(fstr<M> const& rhs) const noexcept -> bool {
    return operator std::string_view() == rhs;
  }

  consteval auto is_blank() const noexcept -> bool {
    // Conforms to `std::isspace`.
    // See: https://en.cppreference.com/w/cpp/string/byte/isspace
    return operator std::string_view().find_first_not_of(" \f\n\r\t\v") == std::string_view::npos;
  }
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
