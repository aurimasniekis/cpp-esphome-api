include(GoogleTest)

set(_esphome_api_test_sources
    test_varint.cpp
    test_message_registry.cpp
    test_wire_golden.cpp
    test_plaintext_frame.cpp
    test_connection_fsm.cpp
    test_sync_client.cpp
    test_entity_store.cpp
    test_entity_handles.cpp
    test_command_builder.cpp
    test_subsystems.cpp
    test_discovery.cpp
)

if(ESPHOME_API_WITH_NOISE)
    list(APPEND _esphome_api_test_sources
        test_noise_vectors.cpp
        test_noise_frame.cpp
    )
endif()

add_executable(esphome_api_tests ${_esphome_api_test_sources})

target_link_libraries(esphome_api_tests
    PRIVATE
        esphome::api
        GTest::gtest_main
)

target_include_directories(esphome_api_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/support)

# Test sources include the generated message_id.hpp directly, so make sure it
# is produced before they compile (the link dependency only orders the build).
if(TARGET esphome_api_message_id)
    add_dependencies(esphome_api_tests esphome_api_message_id)
endif()

esphome_api_set_warnings(esphome_api_tests PRIVATE)
esphome_api_enable_sanitizers(esphome_api_tests)
esphome_api_enable_coverage(esphome_api_tests)
esphome_api_enable_clang_tidy(esphome_api_tests)

gtest_discover_tests(esphome_api_tests)
