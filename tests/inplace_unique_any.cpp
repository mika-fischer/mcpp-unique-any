// Copyright Mika Fischer 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "mcpp/inplace_unique_any.hpp"

#include "doctest/doctest.h"

#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN(cppcoreguidelines-avoid-do-while,misc-use-anonymous-namespace,readability-function-cognitive-complexity)

using mcpp::any_cast;
using mcpp::bad_inplace_unique_any_cast;
using mcpp::inplace_unique_any;

TEST_CASE("default constructed has no value") {
    auto a = inplace_unique_any<32, alignof(std::max_align_t)>();
    CHECK_FALSE(a.has_value());
    CHECK(a.type() == typeid(void));
}

TEST_CASE("construct and access with in-place constructor") {
    auto a = inplace_unique_any<32, alignof(std::max_align_t)>(std::in_place_type<int>, 42);
    CHECK(a.has_value());
    CHECK(a.type() == typeid(int));
    CHECK(any_cast<int>(a) == 42);
}

TEST_CASE("construct and access with forwarding constructor") {
    auto s = std::string("abc");
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::move(s));
    CHECK(a.has_value());
    CHECK(a.type() == typeid(std::string));
    auto *p = any_cast<std::string>(&a);
    REQUIRE(p != nullptr);
    CHECK(*p == "abc");
}

TEST_CASE("initializer_list constructor works") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<std::vector<int>>, {1, 2, 3});
    CHECK(a.has_value());
    CHECK(a.type() == typeid(std::vector<int>));
    auto *v = any_cast<std::vector<int>>(&a);
    REQUIRE(v != nullptr);
    CHECK(v->size() == 3);
    CHECK((*v)[1] == 2);
}

TEST_CASE("move constructor transfers ownership and resets source") {
    auto a = inplace_unique_any<32, alignof(std::max_align_t)>(std::in_place_type<int>, 7);
    auto b = inplace_unique_any<32, alignof(std::max_align_t)>(std::move(a));
    CHECK(b.has_value());
    CHECK_FALSE(a.has_value());
    CHECK(*any_cast<int>(&b) == 7);
}

TEST_CASE("move assignment works and resets old value") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<std::string>, "x");
    auto b = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<std::string>, "y");
    b = std::move(a);
    CHECK(b.has_value());
    CHECK_FALSE(a.has_value());
    CHECK(*any_cast<std::string>(&b) == "x");
}

TEST_CASE("templated assignment works") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<std::string>, "first");
    a = std::string("second");
    CHECK(a.has_value());
    CHECK(a.type() == typeid(std::string));
    CHECK(*any_cast<std::string>(&a) == "second");
}

TEST_CASE("emplace replaces value") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>();
    auto &s = a.emplace<std::string>("hi");
    CHECK(s == "hi");
    CHECK(a.has_value());
    CHECK(a.type() == typeid(std::string));

    auto &v = a.emplace<std::vector<int>>(3, 5);
    CHECK(a.has_value());
    CHECK(a.type() == typeid(std::vector<int>));
    CHECK(v.size() == 3);
    CHECK(v[0] == 5);
}

TEST_CASE("emplace with initializer_list") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>();
    auto &v = a.emplace<std::vector<int>>({4, 5, 6});
    CHECK(v.size() == 3);
    CHECK(v[2] == 6);
}

TEST_CASE("reset clears value") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<int>, 10);
    a.reset();
    CHECK_FALSE(a.has_value());
    CHECK(a.type() == typeid(void));
}

TEST_CASE("swap various cases") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<int>, 1);
    auto b = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<int>, 2);

    // normal swap
    a.swap(b);
    CHECK(*any_cast<int>(&a) == 2);
    CHECK(*any_cast<int>(&b) == 1);

    // swap with empty lhs
    auto c = inplace_unique_any<64, alignof(std::max_align_t)>();
    c.swap(b);
    CHECK(c.has_value());
    CHECK_FALSE(b.has_value());
    CHECK(*any_cast<int>(&c) == 1);

    // swap with both empty
    auto d = inplace_unique_any<64, alignof(std::max_align_t)>();
    auto e = inplace_unique_any<64, alignof(std::max_align_t)>();
    d.swap(e); // should be no-op
    CHECK_FALSE(d.has_value());
    CHECK_FALSE(e.has_value());

    // friend swap
    swap(a, c);
    CHECK(*any_cast<int>(&a) == 1);
    CHECK(*any_cast<int>(&c) == 2);
}

TEST_CASE("any_cast const and non-const pointer overloads") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<int>, 42);
    const auto &ca = a;
    CHECK(any_cast<int>(&a) != nullptr);
    CHECK(any_cast<int>(&ca) != nullptr);
    CHECK(any_cast<double>(&a) == nullptr);
    CHECK(any_cast<double>(&ca) == nullptr);
}

TEST_CASE("any_cast by reference and value") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<int>, 5);
    CHECK(any_cast<int &>(a) == 5);
    CHECK(any_cast<int>(a) == 5);
    CHECK(any_cast<int>(std::move(a)) == 5);
}

TEST_CASE("any_cast throws on wrong type") {
    auto a = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<int>, 1);
    CHECK_THROWS_AS(any_cast<std::string>(a), bad_inplace_unique_any_cast);
    CHECK_THROWS_AS(any_cast<std::string &>(a), bad_inplace_unique_any_cast);
    auto b = inplace_unique_any<64, alignof(std::max_align_t)>(std::in_place_type<std::string>, "s");
    CHECK_THROWS_AS(any_cast<int>(std::move(b)), bad_inplace_unique_any_cast);
}

// NOLINTEND(cppcoreguidelines-avoid-do-while,misc-use-anonymous-namespace,readability-function-cognitive-complexity)