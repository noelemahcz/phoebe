#pragma once


#if defined(_MSC_VER) && (_MSC_VER >= 1929) && !defined(__clang__)
// See: https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
