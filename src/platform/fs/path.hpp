#pragma once

#include <string>
#include <string_view>
#include "util/result.hpp"

namespace mcserver {

// Path utilities with security checks
class Path {
public:
    // Normalize path (resolve . and .., convert separators)
    static std::string normalize(std::string_view path);

    // Join path components
    static std::string join(std::string_view base, std::string_view component);

    // Check if path is absolute
    static bool is_absolute(std::string_view path);

    // Check if path contains traversal attempts (.. escaping)
    static bool has_traversal(std::string_view path);

    // Get directory name
    static std::string dirname(std::string_view path);

    // Get filename
    static std::string filename(std::string_view path);

    // Get extension
    static std::string extension(std::string_view path);

    // Check if path exists
    static bool exists(std::string_view path);

    // Check if path is a directory
    static bool is_directory(std::string_view path);

    // Create directory (and parents if needed)
    static Result<void> create_directories(std::string_view path);

private:
    static char preferred_separator();
};

} // namespace mcserver
