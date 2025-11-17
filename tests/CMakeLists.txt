include(FetchContent)
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/onqtam/doctest
    GIT_TAG        v2.4.12
)
FetchContent_MakeAvailable(doctest)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

add_executable(test-unique-any alloc.cpp inplace_any_view.cpp inplace_unique_any.cpp unique_any.cpp)
target_link_libraries(test-unique-any PRIVATE mcpp::unique-any doctest_with_main)
doctest_discover_tests(test-unique-any)