#include "asio_executor.hpp"

#include <utility>

namespace esphome::api {

void AsioExecutor::post(std::function<void()> fn) {
    asio::post(io_, std::move(fn));
}

TimerId AsioExecutor::schedule_after(const std::chrono::milliseconds delay,
                                     std::function<void()> fn) {
    const TimerId id = next_id_++;
    auto timer = std::make_shared<asio::steady_timer>(io_);
    timer->expires_after(delay);
    timers_.emplace(id, timer);
    timer->async_wait([this, id, fn = std::move(fn)](const std::error_code ec) {
        // Remove ourselves first so the handler may reschedule under the same id
        // space without surprises.
        timers_.erase(id);
        if (!ec) {
            fn();
        }
    });
    return id;
}

void AsioExecutor::cancel(const TimerId id) {
    if (const auto it = timers_.find(id); it != timers_.end()) {
        it->second->cancel();  // fires the wait handler with operation_aborted
    }
}

}  // namespace esphome::api
