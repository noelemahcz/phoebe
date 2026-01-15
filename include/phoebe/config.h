#pragma once


// See: https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/
#if defined(_MSC_VER) && (_MSC_VER >= 1929) /* && !defined(__clang__) */
// NOTE: Clang targeting MSVC (clang-cl) defines _MSC_VER and follows the MSVC ABI.
// It requires `msvc::no_unique_address` for binary compatibility, so we
// treat it the same as MSVC and do not exclude it (no !defined(__clang__)).
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
