# Each example is an executable named esphome_api_<name> (matched by the
# Makefile `examples` target). They require a live device to do anything useful;
# `--help` prints usage and exits without connecting.

set(_esphome_api_examples
    hello
    list_entities
    subscribe_states
    control_light
    logs
    bluetooth_scan
    discover
)

if(ESPHOME_API_WITH_NOISE)
    list(APPEND _esphome_api_examples noise_connect)
endif()

foreach(_ex IN LISTS _esphome_api_examples)
    add_executable(esphome_api_${_ex} ${_ex}.cpp)
    target_link_libraries(esphome_api_${_ex} PRIVATE esphome::api)
    if(TARGET esphome_api_message_id)
        add_dependencies(esphome_api_${_ex} esphome_api_message_id)
    endif()
    esphome_api_set_warnings(esphome_api_${_ex} PRIVATE)
    esphome_api_enable_sanitizers(esphome_api_${_ex})
    esphome_api_enable_coverage(esphome_api_${_ex})
endforeach()
