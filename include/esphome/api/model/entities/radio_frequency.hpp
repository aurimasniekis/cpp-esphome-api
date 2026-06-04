#pragma once

/// @file
/// @brief Typed RadioFrequency entity (info only).

#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/enums.hpp>

#include <cstdint>

namespace esphome::api {

namespace proto {
class ListEntitiesRadioFrequencyResponse;
}  // namespace proto

/// Static description of a radio frequency entity.
struct RadioFrequencyInfo : EntityInfo {
    /// Bitmask of RadioFrequencyCapabilityFlags: bit 0 = transmitter, bit 1 = receiver.
    std::uint32_t capabilities = 0;
    /// Minimum tunable frequency in Hz (0 = unspecified).
    std::uint32_t frequency_min = 0;
    /// Maximum tunable frequency in Hz (0 = unspecified).
    std::uint32_t frequency_max = 0;
    /// Bitmask of supported RadioFrequencyModulation values.
    std::uint32_t supported_modulations = 0;
};

RadioFrequencyInfo parse_radio_frequency_info(const proto::ListEntitiesRadioFrequencyResponse& msg);

}  // namespace esphome::api
