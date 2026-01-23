#pragma once


#include "../config.h"

#include "../meta/fstr.hpp"
#include "../meta/list.hpp"
#include "../meta/misc.hpp"
#include "../meta/pack.hpp"
#include "smf_selection.hpp"

#include <cstddef>

#include <array>
#include <functional>
#include <type_traits>
#include <utility>


namespace phoebe {


// NOTE: `tag_t` should imply in-place semantics.
// template <meta::fstr Tag> struct in_place_tag_t {};
// template <meta::fstr Tag> inline constexpr in_place_tag_t<Tag> in_place_tag{};

template <meta::fstr Tag>
struct tag_t {
  template <meta::fstr RHSTag>
  // clang-format ignore
  consteval auto operator<=>(tag_t<RHSTag> const&) const noexcept { return Tag <=> RHSTag; }

  template <meta::fstr RHSTag>
  // clang-format ignore
  consteval auto operator==(tag_t<RHSTag> const&) const noexcept -> bool { return Tag == RHSTag; }
};

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
  static_assert(!Tag.is_blank(), "tag cannot be empty or whitespace-only");
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

template <meta::fstr Tag, typename Type>
inline constexpr bool is_specialization_of_alt<alt<Tag, Type>> = true;

template <typename Alt>
concept specialization_of_alt = is_specialization_of_alt<Alt>;


namespace variant_detail {

inline constexpr auto npos = static_cast<std::size_t>(-1);

template <meta::fstr Tag, typename... Alts>
consteval auto alt_index_impl() noexcept -> std::size_t {
  constexpr bool matches[] = {Alts::tag == Tag...};
  for (std::size_t i = 0; i < sizeof...(Alts); ++i) {
    if (matches[i]) {
      return i;
    }
  }
  return npos;
}

template <meta::fstr Tag, typename... Alts>
inline constexpr std::size_t alt_pack_index = alt_index_impl<Tag, Alts...>();

template <meta::fstr Tag, typename AltList>
struct alt_list_index_impl {};

template <meta::fstr Tag, typename... Alts>
struct alt_list_index_impl<Tag, meta::list::list<Alts...>> {
  static constexpr std::size_t value = alt_index_impl<Tag, Alts...>();
};

template <meta::fstr Tag, typename AltList>
inline constexpr std::size_t alt_list_index = alt_list_index_impl<Tag, AltList>::value;

template <std::size_t Index, typename... Alts>
struct alt_pack_at : meta::pack::strict::at<Index, Alts...> {};

template <std::size_t Index, typename... Alts>
using alt_pack_at_t = meta::pack::strict::at_t<Index, Alts...>;

template <std::size_t Index, typename AltList>
struct alt_list_at : meta::list::strict::at<Index, AltList> {};

template <std::size_t Index, typename AltList>
using alt_list_at_t = meta::list::strict::at_t<Index, AltList>;

} // namespace variant_detail


// Forward declarations.
namespace variant_detail {
template <std::size_t Index, meta::fstr Tag, typename Type>
struct variant_alt_value;
}


namespace variant_detail::access {

template <std::size_t Index, meta::fstr Tag, typename Type>
struct variant_alt_ref {
  static constexpr std::size_t index = Index;
  static constexpr meta::fstr tag = Tag;
  using type = Type;
  type ref;
};

template <typename VariantAltValue>
constexpr auto variant_alt_ref_from_value(VariantAltValue&& variant_alt_value) noexcept {
  using uncvref_t = std::remove_cvref_t<VariantAltValue>;
  constexpr std::size_t index = uncvref_t::index;
  constexpr meta::fstr tag = uncvref_t::tag;
  using type = meta::forward_cvref_t<VariantAltValue, typename uncvref_t::type>;
  return variant_alt_ref<index, tag, type>{std::forward<VariantAltValue>(variant_alt_value).value};
}

struct storage {
  // WARNING: No bounds checking.
  template <std::size_t Index, typename VariantStorage>
  static constexpr auto&& get_variant_alt(std::in_place_index_t<Index>, VariantStorage&& storage) /*noexcept*/ {
    if constexpr (Index == 0) {
      return std::forward<VariantStorage>(storage).head_;
    } else {
      return get_variant_alt(std::in_place_index<Index - 1>, std::forward<VariantStorage>(storage).tail_);
    }
  }
};

struct base {
  // WARNING: No bounds checking.
  template <std::size_t Index, typename VariantBase>
  static constexpr auto get_variant_alt(VariantBase&& base) /*noexcept*/ {
    // For the case where all alternatives are stateless, directly return `storage_` (which is `none_t`).
    if constexpr (base.has_optimized_storage) {
      using alt_t = alt_list_at_t<Index, typename std::remove_cvref_t<VariantBase>::alt_list>;
      auto&& value = std::forward_like<VariantBase>(base.storage_);
      return variant_alt_ref<Index, alt_t::tag, decltype(value)>{value};

    } else {
      return variant_alt_ref_from_value(
          storage::get_variant_alt(std::in_place_index<Index>, std::forward<VariantBase>(base).storage_));
    }
  }

  // FIXME: Boundary check?
  template <typename Visitor, typename... VariantBases>
  static constexpr decltype(auto) visit_variant_alt(Visitor&& visitor, VariantBases&&... bases) /*noexcept*/ {
    constexpr auto table = make_dispatch_table<Visitor&&, VariantBases&&...>();
    return dispatcher_at(table, bases.which_...)(std::forward<Visitor>(visitor), std::forward<VariantBases>(bases)...);
  }

private:
  template <typename Dispatcher, std::size_t N, typename... Tail>
  static constexpr auto dispatcher_at(std::array<Dispatcher, N> table, std::size_t index, Tail... tail) noexcept {
    if constexpr (sizeof...(Tail) == 0) {
      return table[index];
    } else {
      return dispatcher_at(table[index], tail...);
    }
  }

  template <std::size_t... Is>
  struct dispatcher {
    // FIXME: Boundary check?
    template <typename Visitor, typename... VariantBases>
    static constexpr decltype(auto) dispatch(Visitor visitor, VariantBases... bases) /*noexcept*/ {
      // No deduction happens above, so use `static_cast` instead of `std::forward`.
      return std::invoke(static_cast<Visitor>(visitor), get_variant_alt<Is>(static_cast<VariantBases>(bases))...);
    }
  };

  template <typename Visitor, typename... VariantBases, std::size_t... Is>
  static consteval auto make_dispatch_table_impl(std::index_sequence<Is...>) noexcept {
    return &dispatcher<Is...>::template dispatch<Visitor, VariantBases...>;
  }

  template <typename Visitor, typename... VariantBases, std::size_t... Is, std::size_t... Js, typename... Tail>
  static consteval auto make_dispatch_table_impl(std::index_sequence<Is...> /*accumulator*/,
                                                 std::index_sequence<Js...> /*head*/, Tail... tail) noexcept {
    return std::array{make_dispatch_table_impl<Visitor, VariantBases...>(std::index_sequence<Is..., Js>{}, tail...)...};
  }

  template <typename Visitor, typename... VariantBases>
  static consteval auto make_dispatch_table() noexcept {
    return make_dispatch_table_impl<Visitor, VariantBases...>(
        std::index_sequence<>{}, std::make_index_sequence<std::remove_cvref_t<VariantBases>::size>{}...);
  }
};

struct variant {
  // WARNING: No bounds checking.
  template <std::size_t Index, typename Variant>
  static constexpr auto get_variant_alt(Variant&& variant) /*noexcept*/ {
    return base::get_variant_alt<Index>(std::forward<Variant>(variant).base());
  }

  // FIXME: Boundary check?
  template <typename Visitor, typename... Variants>
  static constexpr decltype(auto) visit_variant_alt(Visitor&& visitor, Variants&&... variants) /*noexcept*/ {
    return base::visit_variant_alt(std::forward<Visitor>(visitor), std::forward<Variants>(variants).base()...);
  }
};

namespace visitors {

template <typename VisitorRef>
struct interleaved {
  VisitorRef visitor;

  template <typename... VariantAltRefs>
  constexpr decltype(auto) operator()(VariantAltRefs const&... alt_refs) const /*noexcept*/ {
    return impl(std::make_index_sequence<2 * sizeof...(VariantAltRefs)>{}, alt_refs...);
  }

  template <std::size_t... Is, typename... VariantAltRefs>
  constexpr decltype(auto) impl(std::index_sequence<Is...>, VariantAltRefs const&... alt_refs) const /*noexcept*/ {
    // Helper lambda to project the parameter pack into an interleaved sequence of (tag, value).
    // The mechanism maps a linear index `I` (from 0 to 2N - 1) to the arguments:
    // - `I / 2` selects the specific `variant_alt_ref` from the pack.
    // - `I % 2 == 0` extracts the `tag` (even indices).
    // - `I % 2 != 0` extracts the `value` reference (odd indices).
    auto const tag_or_value = [this, &alt_refs...]<std::size_t I>() /*noexcept*/ -> decltype(auto) {
      auto const& alt_ref = meta::pack::value::strict::at<I / 2>(alt_refs...);
      if constexpr (I % 2 == 0) /* tag */ { return tag<alt_ref.tag>; }                // clang-format ignore
      else /* value */ { return static_cast<decltype(alt_ref.ref)>(alt_ref.ref); }    // clang-format ignore
    };

    return std::invoke(static_cast<VisitorRef>(visitor), tag_or_value.template operator()<Is>()...);
  }
};

template <typename... CaserRefs>
struct match {
  std::tuple<CaserRefs...> casers;

  template <typename... VariantAltRefs>
  constexpr decltype(auto) operator()(VariantAltRefs const&... alt_refs) const /*noexcept*/ {
    return impl<alt_refs.tag...>(alt_refs...);
  }

  template <meta::fstr... Tags, typename... VariantAltRefs>
  constexpr decltype(auto) impl(VariantAltRefs&&... alt_refs) const /*noexcept*/ {
    auto const& caser = std::apply(
        [](auto const&... casers) /*noexcept*/ -> auto const& { return first_matched<Tags...>(casers...); }, casers);
    static_assert(!std::is_same_v<std::remove_cvref_t<decltype(caser)>, none_t>, "???"); // FIXME: ???
    return std::invoke(static_cast<decltype(caser.func)>(caser.func),
                       static_cast<decltype(alt_refs.ref)>(alt_refs.ref)...);
  }

  template <meta::fstr... Tags>
  static constexpr auto const& first_matched(auto const& caser, auto const&... tail) /*noexcept*/ {
    if constexpr (caser.template matched<Tags...>) {
      return caser;
    } else if constexpr (sizeof...(tail) != 0) {
      return first_matched<Tags...>(tail...);
    } else {
      return none;
    }
  };
};

} // namespace visitors

} // namespace variant_detail::access


template <typename Visitor, typename... Variants>
constexpr decltype(auto) visit(Visitor&& visitor, Variants&&... variants) /*noexcept*/ {
  return variant_detail::access::variant::visit_variant_alt(
      variant_detail::access::visitors::interleaved<Visitor&&>{std::forward<Visitor>(visitor)},
      std::forward<Variants>(variants)...);
}

inline constexpr meta::fstr __ = "";

template <typename FuncRef, meta::fstr... Tags>
struct caser {
  // FIXME: Store value or reference?
  FuncRef func;

  template <meta::fstr... VariantTags>
  static constexpr bool matched = ((Tags == VariantTags || Tags == __) && ...);
};

template <meta::fstr... Tags, typename Func>
constexpr auto case_(Func&& func) noexcept -> caser<Func&&, Tags...> {
  return caser<Func&&, Tags...>{std::forward<Func>(func)};
}

template <typename Caser, typename... Variants>
inline constexpr bool is_eligible_caser = false;

template <typename Func, meta::fstr... Tags, typename... Variants>
inline constexpr bool is_eligible_caser<caser<Func, Tags...>, Variants...> = []<std::size_t... Is>(
                                                                                 std::index_sequence<Is...>) noexcept {
  constexpr bool is_wildcard[] = {Tags == __...};
  constexpr std::size_t indices[] = {
      variant_detail::alt_list_index<Tags, typename std::remove_cvref_t<Variants>::alt_list>...};
  if constexpr (((!is_wildcard[Is] && indices[Is] == variant_detail::npos) || ...)) {
    return false;
  } else {
    return std::invocable<
        Func, meta::forward_cvref_t<
                  Variants, typename std::conditional_t<
                                is_wildcard[Is], std::type_identity<std::type_identity<Variants>>,
                                variant_detail::alt_list_at<
                                    indices[Is], typename std::remove_cvref_t<Variants>::alt_list>>::type::type>...>;
  }
}(std::make_index_sequence<sizeof...(Tags)>{});

template <typename Caser, typename... Variants>
concept eligible_caser = is_eligible_caser<std::remove_cvref_t<Caser>, Variants...>;

template <typename... VariantRefs>
class matcher {
public:
  constexpr explicit matcher(VariantRefs... variants) noexcept : variants_{static_cast<VariantRefs>(variants)...} {}

  template <eligible_caser<VariantRefs...>... Casers>
  constexpr decltype(auto) operator()(Casers const&... casers) && /*noexcept*/ {
    return std::apply(
        [&casers...](VariantRefs... variants) /*noexcept*/ -> decltype(auto) {
          return variant_detail::access::variant::visit_variant_alt(
              variant_detail::access::visitors::match{std::forward_as_tuple(casers...)},
              static_cast<VariantRefs>(variants)...);
        },
        std::move(variants_));
  }

private:
  // FIXME: Store values or references?
  std::tuple<VariantRefs...> variants_;
};

template <typename... Variants>
constexpr auto match(Variants&&... variants) /*noexcept*/ -> matcher<Variants&&...> {
  return matcher<Variants&&...>{std::forward<Variants>(variants)...};
}


namespace variant_detail {

template <std::size_t Index, meta::fstr Tag, typename Type>
struct variant_alt_value {
  static constexpr std::size_t index = Index;
  static constexpr meta::fstr tag = Tag;
  using type = Type;

  // Use `std::in_place_t` to disambiguate from copy/move constructors.
  template <typename... Args>
  constexpr explicit variant_alt_value(std::in_place_t, Args&&... args) /*noexcept*/
      : value(std::forward<Args>(args)...) {}

  type value;
};

// WORKAROUND: MSVC requires a non-empty member to correctly determine the active
// union member during constant evaluation (and even in non-constant evaluation?).
#ifdef PHOEBE_IS_MSVC
template <std::size_t Index, meta::fstr Tag, typename Type>
  requires std::is_empty_v<Type>
struct variant_alt_value<Index, Tag, Type> {
  static constexpr std::size_t index = Index;
  static constexpr meta::fstr tag = Tag;
  using type = Type;

  constexpr explicit variant_alt_value(std::in_place_t) noexcept {}

  NO_UNIQUE_ADDRESS none_t value;
  char dummy = 0;
};
#endif

template <bool TriviallyDestructible, std::size_t Index, typename... Alts>
union variant_storage {};

template <bool TriviallyDestructible, std::size_t Index, typename Alt, typename... Tail>
union variant_storage<TriviallyDestructible, Index, Alt, Tail...> {
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
  /*NO_UNIQUE_ADDRESS*/ variant_alt_value<Index, Alt::tag, typename Alt::type> head_;
  variant_storage<TriviallyDestructible, Index + 1, Tail...> tail_;

  friend struct access::storage;
};

template <typename... Alts>
class variant_base {
public:
  // FIXME: CPS version.
  // template <template <typename...> typename F> using alts_with = F<Alts...>;
  using alt_list = meta::list::list<Alts...>;
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
  using alt_list = base_t::alt_list;
  static constexpr std::size_t size = base_t::size;

  template <meta::fstr Tag, typename... Args, std::size_t AltIndex = variant_detail::alt_pack_index<Tag, Alts...>,
            typename Alt = variant_detail::alt_pack_at_t<AltIndex, Alts...>>
    requires (AltIndex != variant_detail::npos) && // FIXME:
                                                   // Might be redundant because `alt_pack_at_t` is already SFINAE
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
