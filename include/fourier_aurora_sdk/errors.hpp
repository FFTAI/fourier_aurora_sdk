#pragma once

#include <stdexcept>
#include <string>

namespace fourier::aurora_sdk {

class AuroraSdkError : public std::runtime_error {
public:
    explicit AuroraSdkError(const std::string& message) : std::runtime_error(message) {}
};

class ConnectionError : public AuroraSdkError {
public:
    using AuroraSdkError::AuroraSdkError;
};

class InvalidArgumentError : public AuroraSdkError {
public:
    using AuroraSdkError::AuroraSdkError;
};

class InvalidStateError : public AuroraSdkError {
public:
    using AuroraSdkError::AuroraSdkError;
};

}  // namespace fourier::aurora_sdk
