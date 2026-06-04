#pragma once

/// @file
/// @brief Typed Lock entity (info + state + command).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>
#include <esphome/api/proto/proto_fwd.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace esphome::api {

namespace proto {
class ListEntitiesLockResponse;
class LockStateResponse;
class LockCommandRequest;
}  // namespace proto

/// Mirror of proto enum LockCommand (not present in the generated enums mirror).
enum class LockCommand : std::int32_t {
    Unlock = 0,  // LOCK_UNLOCK
    Lock = 1,    // LOCK_LOCK
    Open = 2,    // LOCK_OPEN
};

/// Static description of a lock entity.
struct LockInfo : EntityInfo {
    bool assumed_state = false;
    bool supports_open = false;
    bool requires_code = false;
    std::string code_format;
};

/// A lock's reported state. Named LockEntityState to avoid colliding with the
/// LockState enum class mirrored in enums.hpp.
struct LockEntityState {
    std::uint32_t key = 0;
    LockState state = LockState::None;
};

/// A command targeting a lock entity.
struct LockCommandData {
    std::uint32_t key = 0;
    LockCommand command = LockCommand::Unlock;
    std::optional<std::string> code;
};

LockInfo parse_lock_info(const proto::ListEntitiesLockResponse& msg);
LockEntityState parse_lock_state(const proto::LockStateResponse& msg);
std::unique_ptr<ProtoMessage> to_message(const LockCommandData& cmd);

}  // namespace esphome::api
