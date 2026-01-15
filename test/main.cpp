#include "../include/phoebe/variant/variant.hpp"

#include <string>


using namespace phoebe;


using my_variant = variant<alt<"empty">, alt<"foo", int>, alt<"bar", std::pair<bool, double>>>;
static_assert(!std::is_trivially_move_constructible_v<my_variant>);
static_assert(!std::is_trivially_copy_constructible_v<my_variant>);
static_assert(!std::is_trivially_move_assignable_v<my_variant>);
static_assert(!std::is_trivially_copy_assignable_v<my_variant>);
static_assert(std::is_trivially_move_constructible_v<std::pair<bool, double>>);
static_assert(std::is_trivially_copy_constructible_v<std::pair<bool, double>>);
static_assert(!std::is_trivially_move_assignable_v<std::pair<bool, int>>);
static_assert(!std::is_trivially_copy_assignable_v<std::pair<bool, int>>);

using my_variant2 = variant<alt<"empty">, alt<"str", std::string>>;

using my_variant3 = variant<alt<"empty1">, alt<"empty2">>;


using namespace variant_literals;

inline constexpr my_variant var1_1 = {"empty"_t};
inline constexpr my_variant var1_2 = {"foo"_t, 42};
inline constexpr my_variant var1_3 = {"bar"_t, true, 22.2};

// inline constexpr my_variant2 var2_1 = {"str"_t, std::string(16, 'a')};

inline constexpr my_variant3 var3_1 = {"empty1"_t};
inline constexpr my_variant3 var3_2 = {"empty2"_t};


int main() {
}
