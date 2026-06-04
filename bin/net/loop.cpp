#include "net/loop.hpp"

#include "net/signal.hpp"

namespace cli {

void run_until_signal(esphome::api::SyncClient& client, std::chrono::milliseconds slice) {
    install_signal_handlers();
    while (!stop_requested() && client.is_connected())
        (void)client.async().run_for(slice);
    client.disconnect();
}

}  // namespace cli
