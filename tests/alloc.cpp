
#include "alloc.hpp"

int n_allocs = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto operator new(std::size_t size) -> void * {
    n_allocs += 1;
    return std::malloc(size == 0 ? 1 : size); // NOLINT(cppcoreguidelines-owning-memory,*-no-malloc)
}

void operator delete(void *mem) noexcept {
    n_allocs -= 1;
    std::free(mem); // NOLINT(cppcoreguidelines-owning-memory,*-no-malloc)
}