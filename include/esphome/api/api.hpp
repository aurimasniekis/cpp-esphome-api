#pragma once

/// @file
/// @brief Umbrella header for esphome-api-client. Include this to pull in the
///        public API surface of the library.

#include <esphome/api/bytes.hpp>
#include <esphome/api/client.hpp>
#include <esphome/api/client_options.hpp>
#include <esphome/api/commands/command_builder.hpp>
#include <esphome/api/config.hpp>
#include <esphome/api/connection/connection.hpp>
#include <esphome/api/connection/connection_state.hpp>
#include <esphome/api/discovery.hpp>
#include <esphome/api/exception.hpp>
#include <esphome/api/model/device_info.hpp>
#include <esphome/api/model/entity.hpp>
#include <esphome/api/model/entity_handles.hpp>
#include <esphome/api/model/entity_registry.hpp>
#include <esphome/api/model/entity_store.hpp>
#include <esphome/api/model/entity_type.hpp>
#include <esphome/api/subsystems/bluetooth_proxy.hpp>
#include <esphome/api/subsystems/home_assistant_services.hpp>
#include <esphome/api/subsystems/log_stream.hpp>
#include <esphome/api/subsystems/serial_proxy.hpp>
#include <esphome/api/subsystems/voice_assistant.hpp>
#include <esphome/api/subsystems/zwave_proxy.hpp>
#include <esphome/api/sync_client.hpp>
#include <esphome/api/version.hpp>
