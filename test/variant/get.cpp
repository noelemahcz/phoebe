#include "../../include/phoebe/variant/variant.hpp"

#include <type_traits>


using namespace phoebe::variant_literals;

using phoebe::alt;
using phoebe::variant;
using phoebe::variant_detail::variant_base;
using phoebe::variant_detail::variant_storage;


template <template <typename...> typename F>
using nbi_alts = F<alt<"empty">, alt<"bool", bool>, alt<"int", int>>;

using nbi_var = nbi_alts<variant>;
static_assert(sizeof(nbi_var) == 16);
static_assert(std::is_trivially_destructible_v<nbi_var>);

using nbi_base = nbi_alts<variant_base>;
static_assert(sizeof(nbi_base) == 16);
static_assert(nbi_base::has_storage);

static constexpr nbi_var nbi_n{"empty"_t};
static constexpr nbi_var nbi_b{"bool"_t, true};
static constexpr nbi_var nbi_i{"int"_t, 42};

static_assert(requires { phoebe::variant_detail::access::variant::get_alt<0>(nbi_n); });
static_assert(phoebe::variant_detail::access::variant::get_alt<1>(nbi_b).value == true);
static_assert(phoebe::variant_detail::access::variant::get_alt<2>(nbi_i).value == 42);


template <template <typename...> typename F>
using nnn_alts = F<alt<"empty1">, alt<"empty2">, alt<"empty3">>;

using nnn_var = nnn_alts<variant>;
static_assert(sizeof(nnn_var) == 8);

using nnn_base = nnn_alts<variant_base>;
static_assert(sizeof(nnn_var) == 8);
static_assert(!nnn_base::has_storage);

// static constexpr nnn_var nnn_n1{"empty1"_t};
// static constexpr nnn_var nnn_n2{"empty2"_t};
// static constexpr nnn_var nnn_n3{"empty3"_t};
//
// static_assert(requires { phoebe::variant_detail::access::variant::get_alt<0>(nnn_n1); });
// static_assert(phoebe::variant_detail::access::variant::get_alt<1>(nnn_n2).value == true);
// static_assert(phoebe::variant_detail::access::variant::get_alt<2>(nnn_n3).value == 42);


#include <iostream>

int main() {
  auto x = phoebe::variant_detail::access::variant::get_alt<0>(nbi_n);
  std::cout << &x << std::endl;
}
