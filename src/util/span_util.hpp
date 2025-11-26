#pragma once

#include <span>
#include <cstddef>
#include "types.hpp"

namespace mcserver {

// Convenience type aliases for spans
template<typename T>
using Span = std::span<T>;

template<typename T>
using ConstSpan = std::span<const T>;

using ByteSpan = std::span<byte>;
using ConstByteSpan = std::span<const byte>;

// Helper to create byte spans from arbitrary data
template<typename T>
ByteSpan as_bytes_span(T& data) {
    return std::as_writable_bytes(std::span{&data, 1});
}

template<typename T>
ConstByteSpan as_bytes_span(const T& data) {
    return std::as_bytes(std::span{&data, 1});
}

} // namespace mcserver
