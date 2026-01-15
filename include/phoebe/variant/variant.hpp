#pragma once


#include "../config.h"

#include "../meta/fstr.hpp"
#include "../meta/list.hpp"
#include "../meta/pack.hpp"
#include "meta.hpp" // FIXME: Remove this header.
#include "smf_selection.hpp"

#include <cstddef>

#include <array>
#include <type_traits>
#include <utility>


namespace phoebe {


template <meta::fstr Tag, typename Type = void>
struct alt {
  static constexpr auto tag = Tag;
  using type = Type;
};


template <meta::fstr Tag>
struct tag_t {};

template <meta::fstr Tag>
inline constexpr tag_t<Tag> tag{};

namespace variant_literals {
template <meta::fstr Tag>
consteval auto operator""_t() -> tag_t<Tag> {
  return tag<Tag>;
}
} // namespace variant_literals

// NOTE: `tag_t` should imply in-place semantics.
// template <meta::fstr Tag> struct in_place_tag_t {};
// template <meta::fstr Tag> inline constexpr in_place_tag_t<Tag> in_place_tag{};


namespace variant_detail {

inline constexpr auto npos = static_cast<std::size_t>(-1);

template <meta::fstr Tag, typename... Alts>
consteval auto alt_index_impl() /*noexcept*/ -> std::size_t {
  constexpr bool matches[] = {Alts::tag == Tag...};
  for (std::size_t i = 0; i < sizeof...(Alts); ++i) {
    if (matches[i]) {
      return i;
    }
  }
  return npos;
}

template <meta::fstr Tag, typename... Alts>
inline constexpr std::size_t alt_index = alt_index_impl<Tag, Alts...>();

template <meta::fstr Tag, typename... Alts>
inline constexpr bool tag_in_alts = alt_index<Tag, Alts...> != npos;

template <std::size_t Index, typename... Alts>
using alt_type = meta::pack::strict::at_t<Index, typename Alts::type...>;

} // namespace variant_detail


namespace variant_detail {

template <typename Type>
using is_not_void = std::negation<std::is_void<Type>>;

template <typename... Types>
using non_void_types =
    // meta::filter_t<meta::compose<std::negation, std::is_void>::type, meta::list<Types...>>;
    meta::filter_t<is_not_void, meta::list<Types...>>;

} // namespace variant_detail


namespace variant_detail::access {

struct none_t {};

inline constexpr none_t none{};

struct storage {
  template <typename VariantStorage>
  static constexpr auto&& get_alt(std::in_place_index_t<0>, VariantStorage&& storage) /*noexcept*/ {
    return std::forward<VariantStorage>(storage).head_;
  }

  // WARNING: No bounds checking.
  template <std::size_t Index, typename VariantStorage>
  static constexpr auto&& get_alt(std::in_place_index_t<Index>, VariantStorage&& storage) /*noexcept*/ {
    return get_alt(std::in_place_index<Index - 1>, std::forward<VariantStorage>(storage).tail_);
  }
};

struct base {
  template <std::size_t Index, typename VariantBase, typename UnCVRefBase = std::remove_cvref_t<VariantBase>>
    requires UnCVRefBase::has_storage && (Index < UnCVRefBase::size) // FIXME: Is these checks necessary here?
  static constexpr decltype(auto) get_alt(VariantBase&& base) /*noexcept*/ {
    return storage::get_alt(std::in_place_index<Index>, std::forward<VariantBase>(base).storage_);
  }
};

struct variant {
  template <std::size_t Index, typename Variant>
  // TODO: Constraints.
  static constexpr decltype(auto) get_alt(Variant&& var) /*noexcept*/ {
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

      using alt_t = meta::list2::at_t<active_index, typename std::remove_cvref_t<decltype(base)>::alts>;

      if constexpr (ArgIndex % 2 == 0) /* tag */ {
        return tag<alt_t::tag>;

      } else if (std::is_void_v<typename alt_t::type>) /* void */ {
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

template <std::size_t Index, meta::fstr Tag>
struct variant_alt<Index, alt<Tag>> {
  static constexpr std::size_t index = Index;
  static constexpr meta::fstr tag = Tag;
  using type = void;

  constexpr explicit variant_alt(std::in_place_t) noexcept {}

  // FIXME: Move to `config.h`.
  // WORKAROUND: MSVC requires a non-empty member to correctly
  // determine the active union member during constant evaluation.
#if defined(_MSC_VER) && !defined(__clang__)
  char dummy = 0;
#endif
};

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
  using alts = meta::list<Alts...>;
  static constexpr std::size_t size = sizeof...(Alts);

  static constexpr bool has_storage = !(std::is_void_v<typename Alts::type> && ...);

protected:
  template <std::size_t Index, typename... Args>
    requires (!has_storage)
  constexpr explicit variant_base(std::in_place_index_t<Index>) /*noexcept*/
      : which_(Index) {}

  template <std::size_t Index, typename... Args>
  constexpr explicit variant_base(std::in_place_index_t<Index>, Args&&... args) /*noexcept*/
      : storage_(std::in_place_index<Index>, std::forward<Args>(args)...), which_(Index) {}

private:
  // To avoid unnecessary recursive instantiation of `variant_storage`, which can be expensive for compile times.
  struct empty_t {};
  // FIXME: Exclude `void` types.
  using storage_t = variant_storage<(std::is_trivially_destructible_v<typename Alts::type> && ...), 0, Alts...>;

  NO_UNIQUE_ADDRESS std::conditional_t<has_storage, storage_t, empty_t> storage_;
  std::size_t which_;

  friend struct access::base;
};

} // namespace variant_detail


namespace variant_detail {

template <typename T>
inline constexpr bool is_specialization_of_alt = false;

template <meta::fstr Tag, typename T>
inline constexpr bool is_specialization_of_alt<alt<Tag, T>> = true;

} // namespace variant_detail

template <typename Alt>
concept specialization_of_alt = variant_detail::is_specialization_of_alt<Alt>;


namespace variant_detail {

template <typename... AllTypes>
struct select_smf_ {
  template <typename... NonVoidTypes>
  using type = detail::smf_selection::select_smf<variant_base<AllTypes...>, NonVoidTypes...>;
};

template <typename... Alts>
using select_smf = meta::apply_t<select_smf_<Alts...>::template type, non_void_types<typename Alts::type...>>;

} // namespace variant_detail


template <specialization_of_alt... Alts>
class variant : private
                // FIXME: Use "Conditionally Trivial Special Member Functions" when `__cpp_concepts >= 202002L`.
                variant_detail::select_smf<Alts...> {

  static_assert(sizeof...(Alts) > 0, "variant must consist of at least one alternative.");
  static_assert(meta::has_no_duplicate_strs_v<Alts::tag...>, "variant can not have duplicate tags.");

  using non_void_types = variant_detail::non_void_types<typename Alts::type...>;
  static_assert(!meta::any_v<meta::map_t<std::is_array, non_void_types>>,
                "variant can not have an array type as an alternative.");
  static_assert(!meta::any_v<meta::map_t<std::is_reference, non_void_types>>,
                "variant can not have a reference type as an alternative.");
  static_assert(meta::all_v<meta::map_t<std::is_destructible, non_void_types>>,
                "variant alternatives must be destructible");

  using base_t = variant_detail::select_smf<Alts...>;

public:
  template <meta::fstr Tag, typename... Args, std::size_t Index = variant_detail::alt_index<Tag, Alts...>,
            typename Type = variant_detail::alt_type<Index, Alts...>>
    requires (Index !=
              variant_detail::npos) && // FIXME: Might be redundant because `alt_type` is already SFINAE friendly.
             ((std::is_void_v<Type> && sizeof...(Args) == 0) ||
              (!std::is_void_v<Type> && std::is_constructible_v<Type, Args...>))
  constexpr /*explicit*/ variant(tag_t<Tag>, Args&&... args) noexcept(std::is_void_v<Type> ||
                                                                      std::is_nothrow_constructible_v<Type, Args...>)
      : base_t(std::in_place_index<Index>, std::forward<Args>(args)...) {}

  // TODO: Add overload for `std::initializer_list`.

private:
  friend struct variant_detail::access::variant;

  constexpr auto&& base() & noexcept { return static_cast<base_t&>(*this); }
  constexpr auto&& base() const& noexcept { return static_cast<base_t const&>(*this); }
  constexpr auto&& base() && noexcept { return static_cast<base_t&&>(*this); }
  constexpr auto&& base() const&& noexcept { return static_cast<base_t const&&>(*this); }
};


} // namespace phoebe
