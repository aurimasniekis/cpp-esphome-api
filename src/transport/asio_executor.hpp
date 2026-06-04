#pragma once

/// @file
/// @brief Asio-backed Executor: posts onto an io_context and schedules one-shot
///        steady_timers. Private to the library (includes Asio).

#include <esphome/api/transport/executor.hpp>

#include <asio.hpp>

#include <memory>
#include <unordered_map>

namespace esphome::api {

class AsioExecutor final : public Executor {
public:
    explicit AsioExecutor(asio::io_context& io) : io_(io) {}

    void post(std::function<void()> fn) override;
    TimerId schedule_after(std::chrono::milliseconds delay, std::function<void()> fn) override;
    void cancel(TimerId id) override;

private:
    asio::io_context& io_;
    TimerId next_id_ = 1;
    std::unordered_map<TimerId, std::shared_ptr<asio::steady_timer>> timers_;
};

}  // namespace esphome::api
