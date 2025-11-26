#pragma once

#include "util/types.hpp"
#include <string>
#include <array>

namespace mcserver {

// Represents a UUID (Universally Unique Identifier)
// Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
class UUID {
public:
    UUID();
    explicit UUID(const std::array<u8, 16>& bytes);

    // Generate a UUID from a string (deterministic, based on MD5)
    static UUID from_string(const std::string& str);

    // Generate a random UUID (version 4)
    static UUID random();

    // Parse UUID from string representation
    static UUID from_formatted_string(const std::string& str);

    // Get UUID as formatted string (e.g., "550e8400-e29b-41d4-a716-446655440000")
    std::string to_string() const;

    // Get UUID as filename-safe string (no dashes)
    std::string to_filename() const;

    // Get raw bytes
    const std::array<u8, 16>& get_bytes() const { return bytes_; }

    // Comparison operators
    bool operator==(const UUID& other) const { return bytes_ == other.bytes_; }
    bool operator!=(const UUID& other) const { return bytes_ != other.bytes_; }
    bool operator<(const UUID& other) const { return bytes_ < other.bytes_; }

private:
    std::array<u8, 16> bytes_;
};

} // namespace mcserver
