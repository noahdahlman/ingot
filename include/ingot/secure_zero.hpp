#pragma once

#include <cstddef>
#include <cstring>

namespace ingot {

// Guaranteed-not-elided memory zeroing.
// Uses explicit_bzero (glibc/musl/BSD) which cannot be optimized away.
inline void secure_zero(void* ptr, std::size_t len) noexcept {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    explicit_bzero(ptr, len);
#else
    // Fallback: volatile write + compiler barrier
    auto* p = static_cast<volatile unsigned char*>(ptr);
    for (std::size_t i = 0; i < len; ++i) p[i] = 0;
    asm volatile("" ::: "memory");
#endif
}

} // namespace ingot
