# unique-any

[![Ubuntu](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/ubuntu.yml)
[![Windows](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/windows.yml/badge.svg)](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/windows.yml)
[![macOS](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/macos.yml/badge.svg)](https://github.com/mika-fischer/mcpp-unique-any/actions/workflows/macos.yml)

This is a `std::any`-like type for C++17 that works with move-only types

## Usage
```cpp
auto ptr = std::make_unique<some_class>();   // Something non-copyable
// auto any = std::any(std::move(ptr));      // Compile error!
auto any = mcpp::unique_any(std::move(ptr)); // Works with move-only types
// auto any2 = any;                          // Compile error!
auto any2 = std::move(any);                  // Can be moved
```

Other than working with non-copyable types, and being non-copyable, this works the same as [`std::any`](https://en.cppreference.com/w/cpp/utility/any).


## Future work
- Use vtable approach
- Support no-rtti mode
- Support no-exception mode
- Support C++14?

## License
This software is licensed under the [Boost Software License - Version 1.0](https://www.boost.org/LICENSE_1_0.txt).