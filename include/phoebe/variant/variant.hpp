#pragma once


#include "../config.h"

#include "../meta/fstr.hpp"
#include "../meta/list.hpp"
#include "../meta/pack.hpp"
#include "smf_selection.hpp"

#include <cstddef>

#include <array>
#include <type_traits>
#include <utility>


namespace phoebe {


// NOTE: `tag_t` should imply in-place semantics.
// template <meta::fstr Tag> struct in_place_tag_t {};
// template <meta::fstr Tag> inline constexpr in_place_tag_t<Tag> in_place_tag{};

template <meta::fstr Tag> struct tag_t {};                    // clang-format ignore
template <meta::fstr Tag> inline constexpr tag_t<Tag> tag{};  // clang-format ignore

namespace variant_literals {
template <meta::fstr Tag>
consteval auto operator""_t() noexcept -> tag_t<Tag> { return {}; } // clang-format ignore
} // namespace variant_literals


// Stateless flag, also used for space optimization.
struct none_t {};
inline constexpr none_t none{};


template <meta::fstr Tag, typename Type = none_t>
struct alt {
  // FIXME: Check if the type is complete?
  static_assert(!std::is_array_v<Type>, "array types cannot be used as an alternative");
  static_assert(!std::is_reference_v<Type>, "reference types cannot be used as an alternative");
  static_assert(std::is_destructible_v<Type>, "types used as an alternative must be destructible");

  static constexpr meta::fstr tag = Tag;
  using type = Type;
};

// Treat `void` as `none_t`.
template <meta::fstr Tag>
struct alt<Tag, void> {
  static constexpr meta::fstr tag = Tag;
  using type = none_t;
};

// FIXME: Support for `std::monostate` after C++26.
// // Treat `std::monostate` as `none_t`.
// template <meta::fstr Tag>
// struct alt<Tag, std::monostate> {
//   static constexpr meta::fstr tag = Tag;
//   using type = none_t;
// };

template <typename Alt>
inline constexpr bool is_stateless_alt = std::is_same_v<typename Alt::type, none_t>;


template <typename T>
inline constexpr bool is_specialization_of_alt = false;

template <meta::fstr Tag, typename T>
inline constexpr bool is_specialization_of_alt<alt<Tag, T>> = true;

template <typename Alt>
concept specialization_of_alt = is_specialization_of_alt<Alt>;


namespace variant_detail {

inline constexpr auto npos = static_cast<std::size_t>(-1);

template <meta::fstr Tag, typename... Alts>
consteval auto find_alt_by_tag() /*noexcept*/ -> std::size_t {
  constexpr bool matches[] = {Alts::tag == Tag...};
  for (std::size_t i = 0; i < sizeof...(Alts); ++i) {
    if (matches[i]) {
      return i;
    }
  }
  return npos;
}

template <std::size_t Index, typename... Alts>
using alt_pack_at = meta::pack::strict::at_t<Index, Alts...>;

template <std::size_t Index, typename Alts>
using alt_list_at = meta::list::strict::at_t<Index, Alts>;

} // namespace variant_detail


namespace variant_detail::access {

struct storage {
  // WARNING: No bounds checking.
  template <std::size_t Index, typename VariantStorage>
  static constexpr auto&& get_alt(std::in_place_index_t<Index>, VariantStorage&& storage) /*noexcept*/ {
    if constexpr (Index == 0) {
      return std::forward<VariantStorage>(storage).head_;
    } else {
      return get_alt(std::in_place_index<Index - 1>, std::forward<VariantStorage>(storage).tail_);
    }
  }
};

struct base {
  // WARNING: No bounds checking.
  template <std::size_t Index, typename VariantBase, typename UnCVRefBase = std::remove_cvref_t<VariantBase>>
  static constexpr auto&& get_alt(VariantBase&& base) /*noexcept*/ {
    // For the case where all alternatives are stateless, directly return `storage_` (which is `none_t`).
    if constexpr (UnCVRefBase::has_optimized_storage) {
      return std::forward<VariantBase>(base).storage_;
    } else {
      return storage::get_alt(std::in_place_index<Index>, std::forward<VariantBase>(base).storage_);
    }
  }
};

struct variant {
  // WARNING: No bounds checking.
  template <std::size_t Index, typename Variant>
  static constexpr auto&& get_alt(Variant&& var) /*noexcept*/ {
    return base::get_alt<Index>(std::forward<Variant>(var).base());
  }
};

template <std::size_t... ActiveIndices>
struct dispatcher {
  template <std::size_t... ArgIndices, typename Visitor, typename... VariantBases>
  // FIXME: Boundary check
  // FIXME: Should `std::type_identity_t` be used instead of the raw types?
  static constexpr decltype(auto) dispatch_impl(std::index_sequence<ArgIndices...>, Visitor visitor,
                                                VariantBases... bases) /*noexcept*/ {
    constexpr auto extract = [&bases...]<std::size_t ArgIndex>() /*noexcept*/ {
      constexpr std::size_t base_index = ArgIndex / 2;
      constexpr std::size_t active_index = meta::pack::strict::value::at<base_index>(ActiveIndices...);
      // FIXME: Is `static_cast` necessary?
      auto&& base = meta::pack::strict::value::at<base_index>(static_cast<VariantBases>(bases)...);

      using alt = alt_list_at<active_index, typename std::remove_cvref_t<decltype(base)>::alts>;

      if constexpr (ArgIndex % 2 == 0) /* tag */ {
        return tag<alt::tag>;

      } else if (std::is_void_v<typename alt::type>) /* void */ {
        return none;

      } else /* value */ {
        // FIXME: Is `std::forward` necessary?
        base::get_alt<active_index>(std::forward<decltype(base)>(base));
      }
    };

    // No deduction happens above, so use `static_cast` instead of `std::forward`.
    return std::invoke(static_cast<Visitor>(visitor), extract.template operator()<ArgIndices>()...);
  }

  template <typename Visitor, typename... VariantBases>
  // FIXME: Boundary check
  // FIXME: Should `std::type_identity_t` be used instead of the raw types?
  static constexpr decltype(auto) dispatch(Visitor visitor, VariantBases... bases) /*noexcept*/ {
    using arg_indices_t = std::make_index_sequence<2 * sizeof...(ActiveIndices)>;
    // No deduction happens above, so use `static_cast` instead of `std::forward`.
    // FIXME: Is `static_cast` necessary?
    return dispatch_impl<arg_indices_t, Visitor, VariantBases...>(visitor, bases...);
  }
};

template <typename Visitor, typename... VariantBases, std::size_t... Is>
consteval auto make_dispatch_table_impl(std::index_sequence<Is...>) noexcept {
  return &dispatcher<Is...>::template dispatch<Visitor, VariantBases...>;
}

template <typename Visitor, typename... VariantBases, std::size_t... Is, std::size_t... Js, typename... Tail>
consteval auto make_dispatch_table_impl(std::index_sequence<Is...> /*accumulator*/, std::index_sequence<Js...> /*head*/,
                                        Tail... tail) noexcept {
  return std::array{make_dispatch_table_impl<Visitor, VariantBases...>(std::index_sequence<Is..., Js>{}, tail...)...};
}

template <typename Visitor, typename... VariantBases>
consteval auto make_dispatch_table() noexcept {
  return make_dispatch_table_impl<Visitor>(std::index_sequence<>{},
                                           std::make_index_sequence<std::declval<VariantBases>().size()>{}...);
}

template <typename Dispatcher, std::size_t N>
consteval auto dispatcher_at(std::array<Dispatcher, N> table, std::size_t index) noexcept {
  return table[index];
}

template <typename Dispatcher, std::size_t N, typename... Tail>
consteval auto dispatcher_at(std::array<Dispatcher, N> table, std::size_t index, Tail... tail) noexcept {
  return dispatcher_at(table[index], tail...);
}

template <typename Visitor, typename... VariantBases>
constexpr decltype(auto) visit(Visitor&& visitor, VariantBases&&... bases) /*noexcept*/ {
  constexpr auto table = make_dispatch_table_impl<Visitor&&, VariantBases&&...>();
  return dispatcher_at(table, bases.size()...)(std::forward<Visitor>(visitor), std::forward<VariantBases>(bases)...);
}


} // namespace variant_detail::access


namespace variant_detail {

template <std::size_t Index, typename Alt>
struct variant_alt {
  static constexpr std::size_t index = Index;
  static constexpr meta::fstr tag = Alt::tag;
  using type = Alt::type;

  // Use `std::in_place_t` to disambiguate from copy/move constructors.
  template <typename... Args>
  constexpr explicit variant_alt(std::in_place_t, Args&&... args) /*noexcept*/ : value(std::forward<Args>(args)...) {}

  type value;
};

// WORKAROUND: MSVC requires a non-empty member to correctly determine the active
// union member during constant evaluation (and even in non-constant evaluation?).
#ifdef PHOEBE_IS_MSVC
template <std::size_t Index, typename Alt>
  requires std::is_empty_v<typename Alt::type>
struct variant_alt<Index, Alt> {
  static constexpr std::size_t index = Index;
  static constexpr meta::fstr tag = Alt::tag;
  using type = Alt::type;

  constexpr explicit variant_alt(std::in_place_t) noexcept {}

  NO_UNIQUE_ADDRESS none_t value;
  char dummy = 0;
};
#endif

template <bool TriviallyDestructible, std::size_t Index, typename... Alts>
union variant_storage {};

template <bool TriviallyDestructible, std::size_t Index, typename Alt, typename... Tail>
union variant_storage<TriviallyDestructible, Index, Alt, Tail...> {
public:
  template <typename... Args>
  constexpr explicit variant_storage(std::in_place_index_t<0>, Args&&... args) /*noexcept*/
      : head_(std::in_place, std::forward<Args>(args)...) {}

  template <std::size_t I, typename... Args>
  constexpr explicit variant_storage(std::in_place_index_t<I>, Args&&... args) /*noexcept*/
      : tail_(std::in_place_index<I - 1>, std::forward<Args>(args)...) {}

  variant_storage(variant_storage const&) = default;
  variant_storage(variant_storage&&) = default;
  variant_storage& operator=(variant_storage const&) = default;
  variant_storage& operator=(variant_storage&&) = default;

  constexpr ~variant_storage() requires TriviallyDestructible = default; // clang-format ignore
  constexpr ~variant_storage() {}

private:
  /*NO_UNIQUE_ADDRESS*/ variant_alt<Index, Alt> head_;
  variant_storage<TriviallyDestructible, Index + 1, Tail...> tail_;

  friend struct access::storage;
};

template <typename... Alts>
class variant_base {
public:
  // FIXME: CPS version.
  // template <template <typename...> typename F> using alts_with = F<Alts...>;
  using alts = meta::list::list<Alts...>;
  static constexpr std::size_t size = sizeof...(Alts);

  static constexpr bool has_optimized_storage = (is_stateless_alt<Alts> && ...);

protected:
  template <std::size_t Index, typename... Args>
    requires has_optimized_storage
  constexpr explicit variant_base(std::in_place_index_t<Index>) /*noexcept*/
      : which_(Index) {}

  template <std::size_t Index, typename... Args>
  constexpr explicit variant_base(std::in_place_index_t<Index>, Args&&... args) /*noexcept*/
      : storage_(std::in_place_index<Index>, std::forward<Args>(args)...), which_(Index) {}

private:
  using variant_storage_t = variant_storage<(std::is_trivially_destructible_v<typename Alts::type> && ...), 0, Alts...>;

  // When all alternatives are stateless, avoid unnecessary recursive template instantiations to improve compile times.
  // With the current `get` implementation, this also avoids unnecessary recursive runtime lookups.
  using storage_t = std::conditional_t<has_optimized_storage, none_t, variant_storage_t>;

  NO_UNIQUE_ADDRESS storage_t storage_;
  std::size_t which_;

  friend struct access::base;
};

template <typename... Alts>
using select_smf = detail::smf_selection::select_smf<variant_base<Alts...>, typename Alts::type...>;

} // namespace variant_detail


template <specialization_of_alt... Alts>
class variant : private
                // FIXME: Use "Conditionally Trivial Special Member Functions" when `__cpp_concepts >= 202002L`.
                variant_detail::select_smf<Alts...> {

  static_assert(sizeof...(Alts) > 0, "variant must consist of at least one alternative.");
  static_assert(meta::has_no_duplicate_strs_v<Alts::tag...>, "variant can not have duplicate tags.");

  using base_t = variant_detail::select_smf<Alts...>;

public:
  template <meta::fstr Tag, typename... Args, std::size_t AltIndex = variant_detail::find_alt_by_tag<Tag, Alts...>(),
            typename Alt = variant_detail::alt_pack_at<AltIndex, Alts...>>
    requires (AltIndex != variant_detail::npos) && // FIXME:
                                                   // Might be redundant because `alt_pack_at` is already SFINAE
                                                   // friendly. The template parameter `Alt` is evaluated before this
                                                   // constraint, leading to unfriendly error messages.
             ((is_stateless_alt<Alt> && sizeof...(Args) == 0) ||
              (!is_stateless_alt<Alt> && std::is_constructible_v<typename Alt::type, Args...>))
  constexpr /*explicit*/ variant(tag_t<Tag>, Args&&... args) // clang-format ignore
      noexcept(is_stateless_alt<Alt> || std::is_nothrow_constructible_v<typename Alt::type, Args...>)
      : base_t(std::in_place_index<AltIndex>, std::forward<Args>(args)...) {}

  // TODO: Add overload for `std::initializer_list`.

private:
  constexpr auto&& base() & noexcept { return static_cast<base_t&>(*this); }
  constexpr auto&& base() const& noexcept { return static_cast<base_t const&>(*this); }
  constexpr auto&& base() && noexcept { return static_cast<base_t&&>(*this); }
  constexpr auto&& base() const&& noexcept { return static_cast<base_t const&&>(*this); }

  friend struct variant_detail::access::variant;
};


} // namespace phoebe
