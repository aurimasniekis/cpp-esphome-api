#include <esphome/api/proto/message_id.hpp>
#include <esphome/api/subsystems/home_assistant_services.hpp>

#include "api.pb.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace esphome::api {

namespace {

void set_argument(proto::ExecuteServiceArgument& out, const ServiceValue& value) {
    std::visit(
        [&out](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                out.set_bool_(v);
            } else if constexpr (std::is_same_v<T, std::int32_t>) {
                out.set_int_(v);
            } else if constexpr (std::is_same_v<T, float>) {
                out.set_float_(v);
            } else if constexpr (std::is_same_v<T, std::string>) {
                out.set_string_(v);
            } else if constexpr (std::is_same_v<T, std::vector<bool>>) {
                for (const bool b : v) {
                    out.add_bool_array(b);
                }
            } else if constexpr (std::is_same_v<T, std::vector<std::int32_t>>) {
                for (const std::int32_t i : v) {
                    out.add_int_array(i);
                }
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                for (const float f : v) {
                    out.add_float_array(f);
                }
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                for (const std::string& s : v) {
                    out.add_string_array(s);
                }
            }
        },
        value);
}

}  // namespace

void HomeAssistantServices::on_service(ServiceHandler handler) {
    handler_ = std::move(handler);
    constexpr auto id = static_cast<std::uint32_t>(MessageId::ListEntitiesServicesResponse);
    client_.on(id, [this](const ProtoMessage& msg) {
        const auto& resp = static_cast<const proto::ListEntitiesServicesResponse&>(msg);
        UserService service;
        service.name = resp.name();
        service.key = resp.key();
        service.supports_response = static_cast<SupportsResponseType>(resp.supports_response());
        for (int i = 0; i < resp.args_size(); ++i) {
            UserServiceArg arg;
            arg.name = resp.args(i).name();
            arg.type = static_cast<ServiceArgType>(resp.args(i).type());
            service.args.push_back(std::move(arg));
        }
        services_.push_back(service);
        if (handler_) {
            handler_(service);
        }
    });
}

void HomeAssistantServices::execute(const std::uint32_t key,
                                    const std::vector<ServiceValue>& args,
                                    const bool return_response) {
    proto::ExecuteServiceRequest req;
    req.set_key(key);
    req.set_call_id(next_call_id_++);
    req.set_return_response(return_response);
    for (const ServiceValue& value : args) {
        set_argument(*req.add_args(), value);
    }
    client_.send(req);
}

}  // namespace esphome::api
