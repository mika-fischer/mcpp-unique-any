// Copyright Mika Fischer 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "mcpp/unique_any.hpp"
#include "doctest/doctest.h"
#include <string>

using namespace mcpp;

namespace {

struct small {
    void *a[3];
};

struct large {
    void *a[4];
};

static_assert(sizeof(unique_any) == 4 * sizeof(void *));
static_assert(sizeof(small) + sizeof(void *) <= sizeof(unique_any));
static_assert(sizeof(large) + sizeof(void *) > sizeof(unique_any));

int n_allocs = 0;

} // namespace

auto operator new(std::size_t size) -> void * {
    n_allocs += 1;
    return std::malloc(size == 0 ? 1 : size);
}

void operator delete(void *mem) noexcept {
    n_allocs -= 1;
    std::free(mem);
}

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
    CHECK(n_allocs - pre == 1);

    pre = n_allocs;
    any = {};
    CHECK(n_allocs - pre == -1);
}
