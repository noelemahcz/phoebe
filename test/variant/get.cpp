#include "../../include/phoebe/variant/variant.hpp"

#include <type_traits>


template <typename... Ts>
struct overloads : Ts... {
  using Ts::operator()...;
};


namespace access = phoebe::variant_detail::access;

using namespace phoebe::variant_literals;

using phoebe::tag_t;
using phoebe::tag;
using phoebe::none_t;
using phoebe::alt;
using phoebe::variant;
using phoebe::visit;
using phoebe::variant_detail::variant_alt_value;
using phoebe::variant_detail::variant_base;
using phoebe::variant_detail::variant_storage;

static_assert(sizeof(variant_alt_value<0, "dummy", none_t>) == 1);


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

static_assert(std::is_same_v<decltype((access::variant::get_variant_alt<0>(nbi_n))),
                             access::variant_alt_ref<0, "empty", none_t const&>>);
static_assert(std::is_same_v<decltype((access::variant::get_variant_alt<1>(nbi_b))),
                             access::variant_alt_ref<1, "bool", bool const&>>);
static_assert(std::is_same_v<decltype((access::variant::get_variant_alt<2>(nbi_i))),
                             access::variant_alt_ref<2, "int", int const&>>);

static_assert(access::variant::get_variant_alt<1>(nbi_b).ref == true);
static_assert(access::variant::get_variant_alt<2>(nbi_i).ref == 42);

// Fails to compile on MSVC without a workaround in `variant_alt_value`.
static constexpr auto val = access::variant::get_variant_alt<0>(nbi_n);

// Reference forms always compile, no workaround needed.
static constexpr auto&& ref = access::variant::get_variant_alt<0>(nbi_n);

static constexpr auto nbi_base_visitor = overloads{
    [](access::variant_alt_ref<0, "empty", none_t const&>) -> bool { return false; },
    [](access::variant_alt_ref<1, "bool", bool const&>) -> bool { return true; },
    [](access::variant_alt_ref<2, "int", int const&>) -> bool { return false; },
};
static_assert(access::variant::visit_variant_alt(nbi_base_visitor, nbi_b));

static constexpr auto nbi_visitor = overloads{
    [](tag_t<"empty">, none_t const&) -> bool { return false; },
    [](tag_t<"bool">, bool const&) -> bool { return true; },
    [](tag_t<"int">, int const&) -> bool { return false; },
};
static_assert(visit(nbi_visitor, nbi_b));


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

static_assert(std::is_same_v<decltype(access::variant::get_variant_alt<0>(nnn_n1)),
                             access::variant_alt_ref<0, "empty1", none_t const&>>);
static_assert(std::is_same_v<decltype(access::variant::get_variant_alt<1>(nnn_n2)),
                             access::variant_alt_ref<1, "empty2", none_t const&>>);
static_assert(std::is_same_v<decltype(access::variant::get_variant_alt<2>(nnn_n3)),
                             access::variant_alt_ref<2, "empty3", none_t const&>>);


static constexpr auto nbi_nnn_visitor = [](auto tag1, auto&& value1, auto tag2, auto&& value2) {
  return tag1 == "bool"_t && tag2 == "empty3"_t;
};
static_assert(visit(nbi_nnn_visitor, nbi_b, nnn_n3));
static_assert(!visit(nbi_nnn_visitor, nbi_b, nnn_n1));


#include <iostream>

int main() {
  nbi_var r_nbi_n{"empty"_t};
  nbi_var r_nbi_b{"bool"_t, true};
  nbi_var r_nbi_i{"int"_t, 42};

  auto r_nbi_visitor = overloads{
      [](access::variant_alt_ref<0, "empty", none_t&>) -> bool { return false; },
      [](access::variant_alt_ref<1, "bool", bool&>) -> bool { return true; },
      [](access::variant_alt_ref<2, "int", int&>) -> bool { return false; },
  };

  access::variant::visit_variant_alt(r_nbi_visitor, r_nbi_b);

  // auto x = phoebe::variant_detail::access::variant::get_alt<0>(nbi_n);
  // std::cout << &x << std::endl;
}
