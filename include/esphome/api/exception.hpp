#pragma once

/// @file
/// @brief Exception hierarchy thrown by esphome-api-client.

#include <stdexcept>
#include <string>

namespace esphome::api {

/// Base class for every error raised by the library.
class ApiError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// TCP-level failure: connect refused, reset, EOF, host unreachable.
class ConnectionError : public ApiError {
public:
    using ApiError::ApiError;
};

/// The peer violated the framing or message protocol (bad indicator byte,
/// oversized frame, malformed varint, undecodable payload, ...).
class ProtocolError : public ApiError {
public:
    using ApiError::ApiError;
};

/// The Noise handshake failed (bad PSK, MAC mismatch, unexpected message).
class HandshakeError : public ProtocolError {
public:
    using ProtocolError::ProtocolError;
};

/// The connection's encryption mode does not match the device: a plaintext
/// client reached a device that requires Noise, or an encrypted client reached
/// a plaintext-only device. A HandshakeError so callers that treat encryption
/// setup failures uniformly still catch it, but its message is self-explanatory
/// and names the corrective action (add or drop the PSK).
class EncryptionMismatchError : public HandshakeError {
public:
    using HandshakeError::HandshakeError;
};

/// Authentication was rejected, or the negotiated API version is unsupported.
class AuthenticationError : public ApiError {
public:
    using ApiError::ApiError;
};

/// An operation did not complete before its deadline.
class TimeoutError : public ApiError {
public:
    using ApiError::ApiError;
};

/// A symmetric encrypt/decrypt operation failed (AEAD tag mismatch, bad nonce).
class EncryptionError : public ProtocolError {
public:
    using ProtocolError::ProtocolError;
};

}  // namespace esphome::api
