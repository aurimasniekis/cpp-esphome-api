#pragma once

/// @file
/// @brief Executor + timer abstraction (asio-free). The Asio implementation
///        wraps an io_context; tests use a manually-advanced fake clock.

#include <chrono>
#include <cstdint>
#include <functional>

namespace esphome::api {

/// Opaque handle to a scheduled timer; `invalid_timer` means "no timer".
using TimerId = std::uint64_t;
inline constexpr TimerId invalid_timer = 0;

/// Minimal executor: deferred execution plus one-shot timers. Everything the
/// connection does is funnelled through here so it can run on a real
/// io_context, a strand, or a deterministic test clock.
class Executor {
public:
    virtual ~Executor() = default;

    /// Run `fn` soon, on the executor (never inline / re-entrantly).
    virtual void post(std::function<void()> fn) = 0;

    /// Run `fn` once after `delay`. Returns a handle usable with `cancel`.
    virtual TimerId schedule_after(std::chrono::milliseconds delay, std::function<void()> fn) = 0;

    /// Cancel a pending timer (no-op if it already fired or was cancelled).
    virtual void cancel(TimerId id) = 0;
};

}  // namespace esphome::api
