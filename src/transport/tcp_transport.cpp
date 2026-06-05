#include "tcp_transport.hpp"

#include <utility>

namespace esphome::api {

TcpTransport::TcpTransport(const asio::any_io_executor& executor)
    : socket_(executor), resolver_(executor) {}

TcpTransport::~TcpTransport() {
    close();
}

bool TcpTransport::is_open() const {
    return socket_.is_open();
}

void TcpTransport::async_connect(const std::string& host,
                                 const std::uint16_t port,
                                 ConnectHandler handler) {
    resolver_.async_resolve(
        host,
        std::to_string(port),
        [this, handler = std::move(handler)](
            const std::error_code ec,
            const asio::ip::tcp::resolver::results_type& results) mutable {
            if (ec) {
                handler(ec);
                return;
            }
            asio::async_connect(
                socket_,
                results,
                [this, handler = std::move(handler)](const std::error_code connect_ec,
                                                     const asio::ip::tcp::endpoint&) {
                    if (!connect_ec) {
                        // The ESPHome native API is a stream of many tiny
                        // frames (the Noise handshake alone is several
                        // round-trips). Leaving Nagle's algorithm enabled lets
                        // it coalesce these and interact badly with delayed-ACK
                        // — pronounced on Windows, whose delayed-ACK timer can
                        // stall each frame ~200ms — causing handshake flakiness
                        // and laggy state updates. Disable it like aioesphomeapi
                        // and the firmware do. Best-effort: ignore failures.
                        std::error_code ignored;
                        socket_.set_option(asio::ip::tcp::no_delay(true), ignored);
                    }
                    handler(connect_ec);
                });
        });
}

void TcpTransport::start_read(ReadHandler on_read) {
    read_handler_ = std::move(on_read);
    do_read();
}

void TcpTransport::do_read() {
    socket_.async_read_some(
        asio::buffer(read_buffer_), [this](const std::error_code ec, const std::size_t bytes) {
            if (!read_handler_) {
                return;
            }
            if (ec) {
                read_handler_(ec, ByteView{});
                return;
            }
            read_handler_(std::error_code{}, ByteView(read_buffer_.data(), bytes));
            // The handler may have closed us in response to the data.
            if (socket_.is_open() && read_handler_) {
                do_read();
            }
        });
}

void TcpTransport::async_write(ByteBuffer data, WriteHandler handler) {
    write_queue_.push_back(PendingWrite{std::move(data), std::move(handler)});
    if (!writing_) {
        do_write();
    }
}

void TcpTransport::do_write() {
    if (write_queue_.empty()) {
        writing_ = false;
        return;
    }
    writing_ = true;
    auto& [data, handler] = write_queue_.front();
    asio::async_write(
        socket_, asio::buffer(data), [this](const std::error_code ec, std::size_t /*bytes*/) {
            const auto [done_data, done_handler] = std::move(write_queue_.front());
            write_queue_.pop_front();
            if (done_handler) {
                done_handler(ec);
            }
            if (ec) {
                writing_ = false;
                return;
            }
            do_write();
        });
}

void TcpTransport::close() {
    if (socket_.is_open()) {
        std::error_code ignored;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    }
    resolver_.cancel();
}

}  // namespace esphome::api
