# Phoebe

Phoebe is a C++20 tagged union library that uses string literal tags for type identification. Unlike `std::variant`, which relies on indices or unique types, Phoebe leverages non-type template parameters (NTTP) to provide a more expressive, name-based interface.

## Syntax Overview

Variants are defined by associating string tags with types using the `alt` template. The library provides a custom literal `_t` to produce tag objects at runtime/compile-time.

```cpp
#include <phoebe/variant/variant.hpp>
#include <print>

using namespace phoebe;
using namespace phoebe::variant_literals;

struct Error { std::string message; };

using Result = variant<
    alt<"ok", int>,
    alt<"err", Error>,
    alt<"pending", none_t>
>;

Result res{"ok"_t, 200};
```

## Access Modes

Phoebe supports two distinct ways to access variant data: a high-level **Match DSL** and a traditional **Visitation** API.

### 1. Match DSL

The library features a functional-style matching DSL. It supports single or multiple variants, wildcards, and ensures exhaustiveness at compile time.

**Matching Order:** Patterns are evaluated sequentially from top to bottom, similar to Rust, Haskell, or C#. The first matching case is executed.

```cpp
match(res)(
    // Specific case
    case_<"ok">([](int val) {
        std::println("Success: {}", val);
    }),
    // Fallback for everything else
    case_<__>([](auto&&) {
        std::println("Something else");
    })
);
```

### 2. Traditional Visitation

Phoebe provides a `visit` function similar to `std::visit`, but with a key difference: the visitor receives both the **tag** and the **value**. This allows for flexible dispatch strategies.

#### Using `if constexpr`

You can use a single generic lambda and dispatch based on the tag compile-time.

```cpp
visit([](auto tag, auto&& val) {
    if constexpr (tag == "ok"_t) {
        std::println("OK: {}", val);
    } else if constexpr (tag == "err"_t) {
        std::println("Error: {}", val.message);
    } else {
        std::println("Pending or unknown");
    }
}, res);
```

#### Using Overloads and Default Branches

You can combine specific handlers with a generic fallback (`auto...`) to handle remaining cases.

```cpp
struct Overloader {
    void operator()(tag_t<"ok">, int val) { std::println("Int: {}", val); }

    // Default branch catches "err" and "pending"
    template<typename T>
    void operator()(auto tag, T&& val) {
        std::println("Other: {}", typeid(T).name());
    }
};

visit(Overloader{}, res);
```

## Multi-variant Support

Both `match` and `visit` support joint dispatch over multiple variants.

```cpp
using State = variant<alt<"idle", none_t>, alt<"active", int>>;
using Signal = variant<alt<"start", int>, alt<"stop", none_t>>;

State state{"idle"_t};
Signal signal{"start"_t, 42};

// Match DSL: Checks patterns in declared order
match(state, signal)(
    case_<"idle", "start">([](none_t, int val) { /* ... */ }),
    case_<__, __>([] (auto&&...) { /* Catch-all */ })
);

// Visit: Receives (tag1, val1, tag2, val2)
visit([](auto t1, auto&& v1, auto t2, auto&& v2) {
    if constexpr (t1 == "idle"_t && t2 == "start"_t) {
        // ...
    }
}, state, signal);
```

## Design Considerations

- **Memory Efficiency & Compile Times**: Phoebe leverages `[[no_unique_address]]` for compact layout. Crucially, if all alternatives are stateless (e.g., `none_t`), the storage collapses to a simple integer index. This eliminates the recursive union instantiation entirely, significantly reducing compile times and template bloat.

- **Compile-time Optimized Dispatch**: Visitation uses a compile-time generated dispatch table (multidimensional array), enabling O(1) jump complexity for both single and multi-variant visitation. This approach provides predictable performance and scales naturally to handle the Cartesian product of multiple variants.

- **Zero-Cost Tag Abstraction**: While tags are specified as string literals for readability, they are resolved entirely at compile-time as Non-Type Template Parameters (NTTP). There is no runtime string comparison overhead.

- **Pattern Matching Semantics**: The `match` DSL provides **Exhaustiveness Checking**, ensuring all cases are handled at compile-time. Matching is performed sequentially (top-down), consistent with the behavior of pattern matching in languages like Rust, C#, and Haskell.

## Requirements

- C++20 compliant compiler (GCC 10.1+, Clang 12+, MSVC 19.29+)
- CMake 3.14+ for building tests

The library is header-only. Simply add the `include` directory to your project's include path.
