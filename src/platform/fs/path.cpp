#include "path.hpp"
#include <algorithm>
#include <filesystem>

namespace mcserver {

char Path::preferred_separator() {
#ifdef PLATFORM_WINDOWS
    return '\\';
#else
    return '/';
#endif
}

std::string Path::normalize(std::string_view path) {
    std::string normalized(path);

    // Convert all separators to preferred separator
    std::replace(normalized.begin(), normalized.end(), '/', preferred_separator());
    std::replace(normalized.begin(), normalized.end(), '\\', preferred_separator());

    // Use filesystem library for proper normalization
    std::filesystem::path fs_path(normalized);
    return fs_path.lexically_normal().string();
}

std::string Path::join(std::string_view base, std::string_view component) {
    std::filesystem::path result(base);
    result /= component;
    return result.string();
}

bool Path::is_absolute(std::string_view path) {
    std::filesystem::path fs_path(path);
    return fs_path.is_absolute();
}

bool Path::has_traversal(std::string_view path) {
    std::string normalized = normalize(path);
    return normalized.find("..") != std::string::npos;
}

std::string Path::dirname(std::string_view path) {
    std::filesystem::path fs_path(path);
    return fs_path.parent_path().string();
}

std::string Path::filename(std::string_view path) {
    std::filesystem::path fs_path(path);
    return fs_path.filename().string();
}

std::string Path::extension(std::string_view path) {
    std::filesystem::path fs_path(path);
    return fs_path.extension().string();
}

bool Path::exists(std::string_view path) {
    std::filesystem::path fs_path(path);
    return std::filesystem::exists(fs_path);
}

bool Path::is_directory(std::string_view path) {
    std::filesystem::path fs_path(path);
    return std::filesystem::is_directory(fs_path);
}

Result<void> Path::create_directories(std::string_view path) {
    std::string path_str(path);
    std::filesystem::path fs_path{path_str};

    std::error_code ec;
    if (!std::filesystem::create_directories(fs_path, ec)) {
        if (ec) {
            return ErrorCode::IOError;
        }
    }

    return Result<void>();
}

} // namespace mcserver
