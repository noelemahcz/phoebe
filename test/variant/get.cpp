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
using phoebe::match;
using phoebe::case_;
using phoebe::__;
using phoebe::variant_detail::variant_alt_value;
using phoebe::variant_detail::variant_base;
using phoebe::variant_detail::variant_storage;

static_assert(sizeof(variant_alt_value<0, "dummy", none_t>) == 1);

// using wtf_var = variant<alt<"2">>;
// static constexpr wtf_var ar{"3"_t};


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
  if constexpr (tag1 == "bool"_t && tag2 == "empty3"_t) {
    // ...
    return true;

  } else if constexpr (tag1 == "int"_t && tag2 == "empty1"_t) {
    // ...
    return true;

  } else {
    // otherwise
    return false;
  }
};

static_assert(visit(nbi_nnn_visitor, nbi_b, nnn_n3));
static_assert(visit(nbi_nnn_visitor, nbi_i, nnn_n1));


#include <print>

int main() {
  nbi_var r_nbi_n{"empty"_t};
  nbi_var r_nbi_b{"bool"_t, true};
  nbi_var r_nbi_i{"int"_t, 42};

  auto r_nbi_visitor = overloads{
      [](access::variant_alt_ref<0, "empty", none_t&>) { std::println("empty&"); },
      [](access::variant_alt_ref<1, "bool", bool&>) { std::println("bool&"); },
      [](access::variant_alt_ref<2, "int", int&>) { std::println("int&"); },
      [](access::variant_alt_ref<0, "empty", none_t&&>) { std::println("empty&&"); },
      [](access::variant_alt_ref<1, "bool", bool&&>) { std::println("bool&&"); },
      [](access::variant_alt_ref<2, "int", int&&>) { std::println("int&&"); },
  };
  access::variant::visit_variant_alt(r_nbi_visitor, r_nbi_n);
  access::variant::visit_variant_alt(r_nbi_visitor, r_nbi_b);
  access::variant::visit_variant_alt(r_nbi_visitor, r_nbi_i);
  access::variant::visit_variant_alt(r_nbi_visitor, nbi_var{"empty"_t});
  access::variant::visit_variant_alt(r_nbi_visitor, nbi_var{"bool"_t, false});
  access::variant::visit_variant_alt(r_nbi_visitor, nbi_var{"int"_t, 43});

  auto r_nbi_visitor_2 = overloads{
      [](tag_t<"empty">, auto&& value) { std::println("empty"); },
      [](tag_t<"bool">, auto&& value) { std::println("bool"); },
      [](tag_t<"int">, auto&& value) { std::println("int"); },
  };
  visit(r_nbi_visitor_2, r_nbi_n);
  visit(r_nbi_visitor_2, r_nbi_b);
  visit(r_nbi_visitor_2, r_nbi_i);
  visit(r_nbi_visitor_2, nbi_var{"empty"_t});
  visit(r_nbi_visitor_2, nbi_var{"bool"_t, false});
  visit(r_nbi_visitor_2, nbi_var{"int"_t, 43});

  // clang-format off
  using var = variant<
    alt<"empty">,
    alt<"b", bool>,
    alt<"i", int>,
    alt<"d", double>
  >;

  var var_n{"empty"_t};
  var var_b{"b"_t, true};
  var const c_var_i{"i"_t, 42};

  // match
  match(var_n, var_b, c_var_i, var{"d"_t, 92.2})(
      case_<"empty", "b", "i", "d">([](none_t&, bool& b, int const& i, double&& d) {
        std::println("empty, b({}), i({}), d({})", b, i, d);
      }),
      case_<__, __, __, __>([](auto&&...) { std::println("wildcards"); })
  );

  // mismatch
  match(var_n, var_b, c_var_i, var{"d"_t, 92.2})(
      case_<"empty", "i", "i", "d">([](none_t&, int& i1, int const& i2, double&& d) {
        std::println("empty, i1({}), i2({}), d({})", i1, i2, d);
      }),
      case_<__, __, __, __>([](auto&&...) { std::println("wildcards"); })
  );

  // match in order
  match(var_n, var_b, c_var_i, var{"d"_t, 92.2})(
      case_<__, __, __, __>([](auto&&...) { std::println("wildcards"); }),
      case_<"empty", "b", "i", "d">([](none_t&, bool& b, int const& i, double&& d) {
        std::println("empty, b({}), i({}), d({})", b, i, d);
      })
  );
  // clang-format on

  // match(nbi_var{"int"_t, 42})(case_<"int">([](auto&& value) { std::println("int"); }));

  // std::srand(std::time(nullptr));
  // bool coin_toss = (std::rand() % 2 == 0);
  // nbi_var r_dynamic = coin_toss ? nbi_var{"empty"_t} : nbi_var{"bool"_t, true};
  // visit(r_nbi_visitor_2, r_dynamic);
  //
  // auto r_nbi_visitor_3 = overloads{[](tag_t<"int">, auto&& value) { return value; }, [](auto&&...) { return 0; }};
  // visit(r_nbi_visitor_3, r_nbi_b);

  // auto r_nbi_nnn_visitor = [](auto tag1, auto&& value1, auto tag2, auto&& value2) {
  //   if constexpr (std::is_same_v<decltype(tag1), tag_t<"bool">> && std::is_same_v<decltype(tag2), tag_t<"empty3">>) {
  //     std::println("tag1 == bool({}), tag2 == empty3", value1);
  //   }
  //   // if constexpr (tag1 == "bool"_t && tag2 == "empty3"_t) {
  //   //   std::println("tag1 == bool({}), tag2 == empty3", value1);
  //   // } else if constexpr (tag1 == "int"_t && tag2 == "empty1"_t) {
  //   //   std::println("tag1 == int({}), tag2 == empty1", value1);
  //   // }
  // };

  // auto r_nbi_nnn_visitor = overloads{
  //     [](tag_t<"empty">, auto&& value1, tag_t<"empty1">, auto&& value2) {},
  //     [](tag_t<"empty">, auto&& value1, tag_t<"empty2">, auto&& value2) {},
  //     [](tag_t<"empty">, auto&& value1, tag_t<"empty3">, auto&& value2) {},
  //     [](tag_t<"bool">, auto&& value1, tag_t<"empty1">, auto&& value2) {},
  //     [](tag_t<"bool">, auto&& value1, tag_t<"empty2">, auto&& value2) {},
  //     [](tag_t<"bool">, auto&& value1, tag_t<"empty3">, auto&& value2) {},
  //     [](tag_t<"int">, auto&& value1, tag_t<"empty1">, auto&& value2) {},
  //     [](tag_t<"int">, auto&& value1, tag_t<"empty2">, auto&& value2) {},
  //     [](tag_t<"int">, auto&& value1, tag_t<"empty3">, auto&& value2) {},
  // };
  // visit(r_nbi_nnn_visitor, r_nbi_b, nnn_n3);
  // visit(r_nbi_nnn_visitor, r_nbi_i, nnn_n1);

  // auto x = phoebe::variant_detail::access::variant::get_alt<0>(nbi_n);
  // std::cout << &x << std::endl;
}
