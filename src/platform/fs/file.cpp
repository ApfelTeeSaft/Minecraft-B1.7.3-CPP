#include "file.hpp"
#include <filesystem>

namespace mcserver {

Result<std::vector<byte>> File::read_all_bytes(std::string_view path) {
    std::string path_str(path);
    std::ifstream file{path_str, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
        return ErrorCode::IOError;
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<byte> buffer(static_cast<usize>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return ErrorCode::IOError;
    }

    return buffer;
}

Result<std::string> File::read_all_text(std::string_view path) {
    std::string path_str(path);
    std::ifstream file{path_str};
    if (!file.is_open()) {
        return ErrorCode::IOError;
    }

    std::string content;
    file.seekg(0, std::ios::end);
    content.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    content.assign(std::istreambuf_iterator<char>(file),
                   std::istreambuf_iterator<char>());

    return content;
}

Result<void> File::write_all_bytes(std::string_view path, const byte* data, usize size) {
    std::string path_str(path);
    std::ofstream file{path_str, std::ios::binary};
    if (!file.is_open()) {
        return ErrorCode::IOError;
    }

    if (!file.write(reinterpret_cast<const char*>(data), size)) {
        return ErrorCode::IOError;
    }

    return Result<void>();
}

Result<void> File::write_all_text(std::string_view path, std::string_view text) {
    std::string path_str(path);
    std::ofstream file{path_str};
    if (!file.is_open()) {
        return ErrorCode::IOError;
    }

    if (!file.write(text.data(), text.size())) {
        return ErrorCode::IOError;
    }

    return Result<void>();
}

Result<void> File::append_text(std::string_view path, std::string_view text) {
    std::string path_str(path);
    std::ofstream file{path_str, std::ios::app};
    if (!file.is_open()) {
        return ErrorCode::IOError;
    }

    if (!file.write(text.data(), text.size())) {
        return ErrorCode::IOError;
    }

    return Result<void>();
}

Result<void> File::remove(std::string_view path) {
    std::error_code ec;
    if (!std::filesystem::remove(std::string(path), ec)) {
        if (ec) {
            return ErrorCode::IOError;
        }
    }
    return Result<void>();
}

Result<usize> File::get_size(std::string_view path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(std::string(path), ec);
    if (ec) {
        return ErrorCode::IOError;
    }
    return static_cast<usize>(size);
}

} // namespace mcserver
