#include <esphome/api/model/entities/radio_frequency.hpp>

#include "api.pb.h"
#include "model/entity_common.hpp"

namespace esphome::api {

RadioFrequencyInfo
parse_radio_frequency_info(const proto::ListEntitiesRadioFrequencyResponse& msg) {
    RadioFrequencyInfo info;
    fill_entity_info(info, msg);
    info.capabilities = msg.capabilities();
    info.frequency_min = msg.frequency_min();
    info.frequency_max = msg.frequency_max();
    info.supported_modulations = msg.supported_modulations();
    return info;
}

}  // namespace esphome::api
