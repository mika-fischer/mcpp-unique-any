include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.0.1
)
FetchContent_MakeAvailable(Catch2)
# list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

add_executable(test-unique-any unique_any.cpp)
target_link_libraries(test-unique-any PRIVATE mcpp::unique-any Catch2::Catch2WithMain)
catch_discover_tests(test-unique-any)