#pragma once

/// @file
/// @brief Transport abstraction (asio-free) the connection drives. The Asio TCP
///        implementation lives in the library; tests inject a mock.

#include <esphome/api/bytes.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <system_error>

namespace esphome::api {

/// Byte-stream transport: async connect, a continuous read pump and a
/// serialized write queue. Implementations must invoke every callback on the
/// connection's executor (single-threaded from the connection's perspective).
class Transport {
public:
    using ConnectHandler = std::function<void(std::error_code)>;
    using ReadHandler = std::function<void(std::error_code, ByteView)>;
    using WriteHandler = std::function<void(std::error_code)>;

    virtual ~Transport() = default;

    /// Resolve and connect to `host`:`port`; `handler` fires once.
    virtual void
    async_connect(const std::string& host, std::uint16_t port, ConnectHandler handler) = 0;

    /// Start the continuous read pump. `on_read` is called for each received
    /// chunk, or once with a non-zero error_code on failure/EOF.
    virtual void start_read(ReadHandler on_read) = 0;

    /// Queue `data` for transmission (FIFO). `handler` fires on completion.
    virtual void async_write(ByteBuffer data, WriteHandler handler) = 0;

    /// Close the transport.
    virtual void close() = 0;

    /// Whether the transport is currently open.
    [[nodiscard]] virtual bool is_open() const = 0;
};

}  // namespace esphome::api
