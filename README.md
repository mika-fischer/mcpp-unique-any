# unique-any

[![Ubuntu](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/windows.yml/badge.svg)](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/windows.yml)
[![macOS](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/macos.yml/badge.svg)](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/macos.yml)

`std::any`-like type for C++17 that works with move-only and immovable types.

Other than working with non-copyable types, and being non-copyable, this works the same as [`std::any`](https://en.cppreference.com/w/cpp/utility/any).

## Usage
```cpp
auto move_only = std::make_unique<some_class>();                       // Works with move-only types
auto any1 = mcpp::unique_any(std::move(move_only));
auto any2 = mcpp::unique_any(std::inplace_type<std::atomic<int>>, 42); // Works with immovable types
auto any3 = std::move(any1);                                           // Can be moved
// auto any4 = any2;                                                   // Cannot be copied
```

## Future work
- Support no-rtti mode
- Support no-exception mode
- Support C++14?

## License
This software is licensed under the [Boost Software License - Version 1.0](https://www.boost.org/LICENSE_1_0.txt).