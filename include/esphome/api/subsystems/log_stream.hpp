#pragma once

/// @file
/// @brief Device log streaming subsystem.

#include <esphome/api/model/enums.hpp>
#include <esphome/api/subsystems/subsystem.hpp>

#include <functional>
#include <string>

namespace esphome::api {

/// A single log line streamed from the device.
struct LogEntry {
    LogLevel level = LogLevel::None;
    std::string message;
};

/// Subscribes to and surfaces the device's log stream.
class LogStream : public Subsystem {
public:
    using Handler = std::function<void(const LogEntry&)>;

    explicit LogStream(Client& client) : Subsystem(client) {}

    /// Begin streaming logs at `level` or more severe. `dump_config` asks the
    /// device to also dump its configuration. `handler` receives each line.
    void subscribe(LogLevel level, Handler handler, bool dump_config = false);

private:
    Handler handler_;
};

}  // namespace esphome::api
