#include "../../include/phoebe/variant/variant.hpp"

#include <type_traits>


namespace access = phoebe::variant_detail::access;

using namespace phoebe::variant_literals;

using phoebe::none_t;
using phoebe::alt;
using phoebe::variant;
using phoebe::variant_detail::variant_alt;
using phoebe::variant_detail::variant_base;
using phoebe::variant_detail::variant_storage;

static_assert(sizeof(variant_alt<0, alt<"dummy">>) == 1);


template <template <typename...> typename F>
using nbi_alts = F<alt<"empty">, alt<"bool", bool>, alt<"int", int>>;

using nbi_var = nbi_alts<variant>;
static_assert(sizeof(nbi_var) == 16);
static_assert(std::is_trivially_destructible_v<nbi_var>);

using nbi_base = nbi_alts<variant_base>;
static_assert(sizeof(nbi_base) == 16);
static_assert(!nbi_base::has_optimized_storage);

static constexpr nbi_var nbi_n{"empty"_t};
static constexpr nbi_var nbi_b{"bool"_t, true};
static constexpr nbi_var nbi_i{"int"_t, 42};

static_assert(std::is_same_v<decltype((access::variant::get_alt<0>(nbi_n).value)), none_t const&>);
static_assert(access::variant::get_alt<1>(nbi_b).value == true);
static_assert(access::variant::get_alt<2>(nbi_i).value == 42);

// Fails to compile on MSVC without a workaround in `variant_alt`.
static constexpr auto val = access::variant::get_alt<0>(nbi_n);

// Reference forms always compile, no workaround needed.
static constexpr auto&& ref = access::variant::get_alt<0>(nbi_n);


template <template <typename...> typename F>
using nnn_alts = F<alt<"empty1">, alt<"empty2">, alt<"empty3">>;

using nnn_var = nnn_alts<variant>;
static_assert(sizeof(nnn_var) == 8);
static_assert(std::is_trivially_destructible_v<nnn_var>);

using nnn_base = nnn_alts<variant_base>;
static_assert(sizeof(nnn_base) == 8);
static_assert(nnn_base::has_optimized_storage);

static constexpr nnn_var nnn_n1{"empty1"_t};
static constexpr nnn_var nnn_n2{"empty2"_t};
static constexpr nnn_var nnn_n3{"empty3"_t};

static_assert(std::is_same_v<decltype(access::variant::get_alt<0>(nnn_n1)), none_t const&>);
static_assert(std::is_same_v<decltype(access::variant::get_alt<0>(nnn_n2)), none_t const&>);
static_assert(std::is_same_v<decltype(access::variant::get_alt<0>(nnn_n3)), none_t const&>);


// TODO: Support storage optimization for user-defined empty types.
struct empty_a {};
struct empty_b {};
struct empty_c {};

template <template <typename...> typename F>
using abc_alts = F<alt<"a", empty_a>, alt<"b", empty_b>, alt<"c", empty_c>>;

using abc_var = abc_alts<variant>;
// static_assert(sizeof(abc_var) == 8);
static_assert(std::is_trivially_destructible_v<abc_var>);

using abc_base = abc_alts<variant_base>;
// static_assert(sizeof(abc_base) == 8);
// static_assert(abc_base::has_optimized_storage);


#include <iostream>

int main() {
  // auto x = phoebe::variant_detail::access::variant::get_alt<0>(nbi_n);
  // std::cout << &x << std::endl;
}
