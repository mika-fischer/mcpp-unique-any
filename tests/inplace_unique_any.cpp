// Copyright Mika Fischer 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "mcpp/inplace_unique_any.hpp"
#include "alloc.hpp"
#include "doctest/doctest.h"
#include <memory>
#include <string>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-avoid-do-while,misc-use-anonymous-namespace)

using namespace mcpp;

namespace {

struct small {
    std::array<void *, 3> a;
};

struct large {
    std::array<void *, 4> a;
};

static_assert(sizeof(inplace_unique_any<sizeof(large), alignof(large)>) == sizeof(large) + sizeof(void *));

using unique_any = inplace_unique_any<sizeof(large), alignof(large)>;

} // namespace

TEST_CASE("basic") {
    auto any = unique_any();
    CHECK(!any.has_value());
    CHECK(any.type() == typeid(void));

    auto ptr = std::make_unique<std::string>("Foo");
    CHECK(ptr != nullptr);
    any = std::move(ptr);
    CHECK(any.has_value());
    CHECK(any.type() == typeid(decltype(ptr)));
    CHECK(ptr == nullptr);

    auto any2 = std::move(any);
    CHECK(!any.has_value());
    CHECK(*any_cast<decltype(ptr) &>(any2) == "Foo");
    any = std::move(any2);

    CHECK(*any_cast<decltype(ptr) &>(any) == "Foo");
    ptr = any_cast<decltype(ptr)>(std::move(any));
    CHECK(*ptr == "Foo");
    CHECK(any.has_value());
    CHECK(any_cast<decltype(ptr) &>(any) == nullptr);
}

TEST_CASE("small_buffer") {
    auto pre = n_allocs;
    auto any = unique_any(small{});
    CHECK(n_allocs - pre == 0);
}

TEST_CASE("allocating") {
    auto pre = n_allocs;
    auto any = unique_any(large{});
    CHECK(n_allocs - pre == 0);

    pre = n_allocs;
    any = {};
    CHECK(n_allocs - pre == 0);
}

// TEST_CASE("immovable") {
//     auto any = unique_any(std::in_place_type<std::atomic<int>>, 42);
//     CHECK(any.has_value());
//     CHECK(any_cast<std::atomic<int> &>(any) == 42);
//     auto any2 = std::move(any);
//     CHECK(!any.has_value());
//     CHECK(any2.has_value());
//     CHECK(any_cast<std::atomic<int> &>(any2) == 42);
// }

// NOLINTEND(cppcoreguidelines-avoid-do-while,misc-use-anonymous-namespace)