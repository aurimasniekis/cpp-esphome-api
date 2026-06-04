#pragma once

/// @file
/// @brief Home Assistant user-defined services subsystem.

#include <esphome/api/model/enums.hpp>
#include <esphome/api/subsystems/subsystem.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace esphome::api {

/// One argument of a user-defined service.
struct UserServiceArg {
    std::string name;
    ServiceArgType type = ServiceArgType::Bool;
};

/// A user-defined service advertised by the device (api: `api.services`).
struct UserService {
    std::string name;
    std::uint32_t key = 0;
    std::vector<UserServiceArg> args;
    SupportsResponseType supports_response = SupportsResponseType::None;
};

/// A value passed to a service invocation. The active alternative must match
/// the corresponding argument's declared type.
using ServiceValue = std::variant<bool,
                                  std::int32_t,
                                  float,
                                  std::string,
                                  std::vector<bool>,
                                  std::vector<std::int32_t>,
                                  std::vector<float>,
                                  std::vector<std::string>>;

/// Discovers user-defined services and invokes them.
class HomeAssistantServices : public Subsystem {
public:
    using ServiceHandler = std::function<void(const UserService&)>;

    explicit HomeAssistantServices(Client& client) : Subsystem(client) {}

    /// Register a callback fired for each ListEntitiesServicesResponse (call
    /// before list_entities()). Discovered services accumulate in services().
    void on_service(ServiceHandler handler);

    /// Invoke a service by key with positional arguments.
    void
    execute(std::uint32_t key, const std::vector<ServiceValue>& args, bool return_response = false);

    [[nodiscard]] const std::vector<UserService>& services() const noexcept {
        return services_;
    }

private:
    ServiceHandler handler_;
    std::vector<UserService> services_;
    std::uint32_t next_call_id_ = 1;
};

}  // namespace esphome::api
