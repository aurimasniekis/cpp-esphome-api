#include "commands/serial_server.hpp"
#include "net/signal.hpp"

#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

#ifndef _WIN32

#include "commands/actions/entity_actions.hpp"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace cli {
namespace {

/// Write the whole buffer to `fd`, retrying on EINTR/EAGAIN (PTY back-pressure).
bool full_write(const int fd, const std::string& data) {
    std::size_t off = 0;
    while (off < data.size()) {
        const ssize_t n = ::write(fd, data.data() + off, data.size() - off);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                pollfd pfd{fd, POLLOUT, 0};
                ::poll(&pfd, 1, 50);
                continue;
            }
            return false;
        }
        off += static_cast<std::size_t>(n);
    }
    return true;
}

}  // namespace

int serial_server(const CliContext& ctx,
                  esphome::api::SyncClient& client,
                  const std::uint32_t instance,
                  const esphome::api::SerialProxyConfig& initial,
                  const std::vector<std::string>& args) {
    esphome::api::SerialProxyConfig cfg = initial;
    cfg.instance = instance;
    std::string link = "/tmp/esphome-tty." + std::to_string(instance);
    bool have_rts = false;
    bool have_dtr = false;
    esphome::api::SerialProxyLineStates lines;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= args.size())
                throw std::invalid_argument(a + " requires a value");
            return args[++i];
        };
        if (a == "--link") {
            link = next();
        } else if (a == "--baud") {
            cfg.baudrate = parse_uint(next());
        } else if (a == "--stop") {
            cfg.stop_bits = parse_uint(next());
        } else if (a == "--data") {
            cfg.data_size = parse_uint(next());
        } else if (a == "--rts") {
            lines.rts = parse_bool(next());
            have_rts = true;
        } else if (a == "--dtr") {
            lines.dtr = parse_bool(next());
            have_dtr = true;
        } else {
            throw std::invalid_argument("unknown server option: " + a);
        }
    }

    // Open and prepare a pseudo-terminal master.
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        std::cerr << "error: posix_openpt failed\n";
        return 6;
    }
    if (::grantpt(master) != 0 || ::unlockpt(master) != 0) {
        std::cerr << "error: grantpt/unlockpt failed\n";
        ::close(master);
        return 6;
    }
    const char* slave = ::ptsname(master);
    if (slave == nullptr) {
        std::cerr << "error: ptsname failed\n";
        ::close(master);
        return 6;
    }
    const std::string slave_path = slave;

    termios tio{};
    if (::tcgetattr(master, &tio) == 0) {
        ::cfmakeraw(&tio);
        ::tcsetattr(master, TCSANOW, &tio);
    }
    // fcntl is a variadic POSIX syscall; there is no non-vararg alternative.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    const int flags = ::fcntl(master, F_GETFL, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    ::fcntl(master, F_SETFL, flags | O_NONBLOCK);

    ::unlink(link.c_str());
    if (::symlink(slave_path.c_str(), link.c_str()) != 0)
        ctx.log->warn("could not create symlink {} -> {}", link, slave_path);

    // Configure the remote UART and start streaming.
    client.serial().configure(cfg);
    const esphome::api::SerialPort port = client.serial_proxy(instance);
    if (have_rts || have_dtr)
        port.set_modem_pins(lines);
    client.serial().on_data(
        instance, [master](const esphome::api::SerialProxyData& d) { full_write(master, d.data); });
    port.subscribe();

    std::cerr << "serial bridge ready: " << link << " -> instance " << instance << " @ "
              << cfg.baudrate << " baud (Ctrl-C to stop)\n";

    install_signal_handlers();
    std::array<char, 4096> buf{};
    while (!stop_requested() && client.is_connected()) {
        client.async().run_for(std::chrono::milliseconds(10));
        pollfd pfd{master, POLLIN, 0};
        if (::poll(&pfd, 1, 10) > 0 && (pfd.revents & POLLIN) != 0) {
            if (const ssize_t n = ::read(master, buf.data(), buf.size()); n > 0)
                port.write(std::string(buf.data(), static_cast<std::size_t>(n)));
        }
    }

    ::unlink(link.c_str());
    ::close(master);
    client.disconnect();
    return 0;
}

}  // namespace cli

#else  // _WIN32

namespace cli {

int serial_server(const CliContext&,
                  esphome::api::SyncClient&,
                  std::uint32_t,
                  const esphome::api::SerialProxyConfig&,
                  const std::vector<std::string>&) {
    std::cerr << "error: serial-proxy server (PTY bridge) is POSIX-only\n";
    return 6;
}

}  // namespace cli

#endif
