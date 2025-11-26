#pragma once

#include <utility>
#include <type_traits>
#include <cassert>

namespace mcserver {

// Error types
enum class ErrorCode {
    Success = 0,
    NetworkError,
    IOError,
    ParseError,
    InvalidArgument,
    OutOfMemory,
    Timeout,
    NotFound,
    AlreadyExists,
    PermissionDenied,
    Unknown
};

// Result type for error handling without exceptions
template<typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)), error_(ErrorCode::Success) {}
    Result(ErrorCode error) : error_(error) {
        assert(error != ErrorCode::Success);
    }

    bool is_ok() const { return error_ == ErrorCode::Success; }
    bool is_error() const { return error_ != ErrorCode::Success; }

    T& value() {
        assert(is_ok());
        return value_;
    }

    const T& value() const {
        assert(is_ok());
        return value_;
    }

    T value_or(T default_value) const {
        return is_ok() ? value_ : default_value;
    }

    ErrorCode error() const {
        return error_;
    }

    explicit operator bool() const {
        return is_ok();
    }

private:
    T value_{};
    ErrorCode error_;
};

// Specialization for void
template<>
class Result<void> {
public:
    Result() : error_(ErrorCode::Success) {}
    Result(ErrorCode error) : error_(error) {}

    bool is_ok() const { return error_ == ErrorCode::Success; }
    bool is_error() const { return error_ != ErrorCode::Success; }

    ErrorCode error() const { return error_; }

    explicit operator bool() const { return is_ok(); }

private:
    ErrorCode error_;
};

} // namespace mcserver
