#pragma once

#include <cstddef>
#include <cstdlib>

extern int n_allocs; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
extern auto operator new(std::size_t size) -> void *;
extern void operator delete(void *mem) noexcept;