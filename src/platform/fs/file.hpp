#pragma once

#include <fstream>
#include <vector>
#include <string_view>
#include "util/types.hpp"
#include "util/result.hpp"

namespace mcserver {

// File I/O utilities
class File {
public:
    // Read entire file into buffer
    static Result<std::vector<byte>> read_all_bytes(std::string_view path);

    // Read entire file as text
    static Result<std::string> read_all_text(std::string_view path);

    // Write bytes to file
    static Result<void> write_all_bytes(std::string_view path, const byte* data, usize size);

    // Write text to file
    static Result<void> write_all_text(std::string_view path, std::string_view text);

    // Append text to file
    static Result<void> append_text(std::string_view path, std::string_view text);

    // Delete file
    static Result<void> remove(std::string_view path);

    // Get file size
    static Result<usize> get_size(std::string_view path);
};

} // namespace mcserver
