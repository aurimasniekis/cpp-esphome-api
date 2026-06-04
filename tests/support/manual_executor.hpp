#pragma once

/// @file
/// @brief Deterministic test Executor: a virtual clock the test advances by
///        hand, with no real threads or io_context.

#include <esphome/api/transport/executor.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace esphome::api::testing {

/// Executor whose "time" only moves when the test calls advance(). Posted
/// callbacks run on drain(); timers fire in deadline order during advance().
class ManualExecutor final : public Executor {
public:
    void post(std::function<void()> fn) override {
        ready_.push_back(std::move(fn));
    }

    TimerId schedule_after(const std::chrono::milliseconds delay,
                           std::function<void()> fn) override {
        const TimerId id = next_id_++;
        timers_.emplace(id, Timer{now_ms_ + delay.count(), std::move(fn)});
        return id;
    }

    void cancel(const TimerId id) override {
        timers_.erase(id);
    }

    /// Run every ready posted callback, including ones they enqueue.
    void drain() {
        while (!ready_.empty()) {
            std::function<void()> fn = std::move(ready_.front());
            ready_.erase(ready_.begin());
            fn();
        }
    }

    /// Advance the virtual clock by `delta`, firing due timers earliest-first.
    void advance(const std::chrono::milliseconds delta) {
        now_ms_ += delta.count();
        for (;;) {
            auto best = timers_.end();
            for (auto it = timers_.begin(); it != timers_.end(); ++it) {
                if (it->second.deadline_ms <= now_ms_ &&
                    (best == timers_.end() || it->second.deadline_ms < best->second.deadline_ms)) {
                    best = it;
                }
            }
            if (best == timers_.end()) {
                break;
            }
            std::function<void()> fn = std::move(best->second.fn);
            timers_.erase(best);
            fn();
            drain();
        }
    }

    [[nodiscard]] std::size_t pending_timers() const noexcept {
        return timers_.size();
    }

private:
    struct Timer {
        std::int64_t deadline_ms;
        std::function<void()> fn;
    };

    std::int64_t now_ms_ = 0;
    TimerId next_id_ = 1;
    std::map<TimerId, Timer> timers_;
    std::vector<std::function<void()>> ready_;
};

}  // namespace esphome::api::testing
