// std::getenv (used by the opt-in IO trace below) is a portable standard
// function, but MSVC flags it as C4996; silence it for this translation unit.
// Must precede any include that pulls in <cstdlib>.
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "tcp_transport.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <utility>

namespace esphome::api {

namespace {

// Opt-in IO tracing (set ESPHOME_API_TRACE_IO=1). For every socket read it
// prints, to stderr, how long we waited for the read to complete (`wait` —
// time spent below us: asio/IOCP/network/loop availability) and how long our
// read handler ran (`handle` — decrypt/dispatch and, for the CLI, output).
// This localises the "bursty updates on Windows" stall: a large `wait` means
// data isn't reaching us (loop blocked, VPN/network batching); a large
// `handle` means our processing/output is the bottleneck.
bool io_trace_enabled() {
    static const bool on = std::getenv("ESPHOME_API_TRACE_IO") != nullptr;
    return on;
}

using TraceClock = std::chrono::steady_clock;
TraceClock::time_point g_trace_last_end{};  // when the previous read handler returned

double trace_ms(const TraceClock::duration d) {
    return std::chrono::duration<double, std::milli>(d).count();
}

void trace_read(const std::error_code& ec,
                const std::size_t bytes,
                const TraceClock::time_point arrival) {
    if (!io_trace_enabled()) {
        return;
    }
    const auto now = TraceClock::now();
    const double wait = g_trace_last_end.time_since_epoch().count() == 0
                            ? 0.0
                            : trace_ms(arrival - g_trace_last_end);
    std::cerr << "[esphome-io] wait=" << std::fixed << std::setprecision(1) << wait
              << "ms handle=" << trace_ms(now - arrival) << "ms bytes=" << bytes
              << (ec ? " (ec)" : "") << '\n';
    g_trace_last_end = now;
}

}  // namespace

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
            const auto arrival = TraceClock::now();
            if (!read_handler_) {
                return;
            }
            if (ec) {
                read_handler_(ec, ByteView{});
                trace_read(ec, bytes, arrival);
                return;
            }
            read_handler_(std::error_code{}, ByteView(read_buffer_.data(), bytes));
            trace_read(ec, bytes, arrival);
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
