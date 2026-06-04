#pragma once

/// @file
/// @brief Common base for the `Client`-owned subsystems.

#include <esphome/api/client.hpp>

namespace esphome::api {

/// Shared base for the subsystems exposed by `Client` (log streaming, the
/// various proxies, voice assistant, ...). Each subsystem keeps a back-reference
/// to its owning `Client` to send requests and register message handlers; this
/// base centralizes that wiring so the concrete subsystems only carry their own
/// state.
///
/// A subsystem is owned by — and never outlives — its `Client`, and is bound to
/// that one client for its whole lifetime, hence the reference member.
class Subsystem {
public:
    explicit Subsystem(Client& client) : client_(client) {}

protected:
    // The owning client. A reference (not a pointer) because the binding is
    // fixed at construction and a subsystem is intentionally non-assignable.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    Client& client_;
};

}  // namespace esphome::api
