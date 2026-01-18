#pragma once


#if defined(__clang__) && defined(_MSC_VER)
#define PHOEBE_IS_CLANG_CL 1

#elif defined(__clang__)
#define PHOEBE_IS_CLANG 1

#elif defined(_MSC_VER)
#define PHOEBE_IS_MSVC 1

#elif defined(__GNUC__)
#define PHOEBE_IS_GCC 1

#else
#error "Unknown Compiler"
#endif


/*
 * P2564R3 introduces "Immediate Escalation", which allows constexpr functions (mostly templates)
 * to implicitly promote to consteval if they invoke an immediate function.
 *
 * Without this feature, calling a consteval function from a constexpr context is a compilation error
 * (the "consteval coloring problem"). We fall back to constexpr on older compilers to ensure
 * that the code compiles, effectively relaxing the immediate execution requirement.
 *
 * See: https://wg21.link/P2564
 */
#if defined(__cpp_consteval) && __cpp_consteval >= 202211L
#define CONSTEVAL consteval
#else
#define CONSTEVAL constexpr
#endif


// See: https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/
#if defined(_MSC_VER) && (_MSC_VER >= 1929) /* && !defined(__clang__) */
// NOTE:
// Clang targeting MSVC defines _MSC_VER and follows the MSVC ABI.
// It requires `msvc::no_unique_address` for binary compatibility, so we
// treat it the same as MSVC and do not exclude it (no `!defined(__clang__)`).
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
