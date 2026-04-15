#pragma once

#include <cstddef>
#include <cstring>

#if defined(_MSC_VER)
#include <windows.h>
#endif

namespace ingot {

// Guaranteed-not-elided memory zeroing.
inline void secure_zero(void* ptr, std::size_t len) noexcept {
#if defined(_MSC_VER)
    SecureZeroMemory(ptr, len);
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    explicit_bzero(ptr, len);
#elif defined(__GNUC__) || defined(__clang__)
    auto* p = static_cast<volatile unsigned char*>(ptr);
    for (std::size_t i = 0; i < len; ++i) p[i] = 0;
    asm volatile("" ::: "memory");
#else
    // Weakest fallback: volatile writes prevent per-byte elision.
    // No compiler barrier — may still be reordered, but won't be removed.
    auto* p = static_cast<volatile unsigned char*>(ptr);
    for (std::size_t i = 0; i < len; ++i) p[i] = 0;
#endif
}

} // namespace ingot
