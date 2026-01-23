add_executable(test
    variant/get.cpp
    main.cpp)

# NOTE: `std::print` and `std::println` (requires linking with -lstdc++exp on Windows).
# https://gcc.gnu.org/gcc-14/changes.html#libstdcxx
target_link_libraries(test PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<BOOL:${WIN32}>>:stdc++exp>)
