// Copyright Mika Fischer 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "mcpp/unique_any.hpp"
#include <string>

// runtime tests
auto main() -> int {
    auto ptr = std::make_unique<std::string>("Foo");
    auto any = mcpp::unique_any(std::move(ptr));
    assert(any.has_value());
    assert(*mcpp::any_cast<decltype(ptr) &>(any) == "Foo");
}