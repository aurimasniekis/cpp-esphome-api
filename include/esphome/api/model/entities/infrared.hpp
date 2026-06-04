#pragma once

/// @file
/// @brief Typed Infrared entity (info only).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>

#include <cstdint>

namespace esphome::api {

namespace proto {
class ListEntitiesInfraredResponse;
}  // namespace proto

/// Static description of an infrared entity.
struct InfraredInfo : EntityInfo {
    /// Bitfield of InfraredCapabilityFlags.
    std::uint32_t capabilities = 0;
    /// Demodulation frequency of the IR receiver in Hz (0 = unspecified).
    std::uint32_t receiver_frequency = 0;
};

InfraredInfo parse_infrared_info(const proto::ListEntitiesInfraredResponse& msg);

}  // namespace esphome::api
