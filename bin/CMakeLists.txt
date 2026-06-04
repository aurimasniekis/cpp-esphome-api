# esphome-cli — the command-line tool. Built only when ESPHOME_API_BUILD_CLI is
# ON. Its dependencies (CLI11 / spdlog / nlohmann_json) are PRIVATE here and
# never reach the esphome::api library target.

set(ESPHOME_CLI_SOURCES
    main.cpp
    app/global_options.cpp
    app/dispatch.cpp
    app/help.cpp
    config/config.cpp
    config/xdg.cpp
    io/output.cpp
    io/yaml_emit.cpp
    io/json_model.cpp
    net/connection.cpp
    net/signal.cpp
    net/loop.cpp
    commands/domains.cpp
    commands/glob.cpp
    commands/actions/entity_actions.cpp
    commands/cmd_scan.cpp
    commands/cmd_config.cpp
    commands/cmd_devices.cpp
    commands/cmd_info.cpp
    commands/cmd_list.cpp
    commands/cmd_entity.cpp
    commands/cmd_logs.cpp
    commands/cmd_watch.cpp
    commands/cmd_bt.cpp
    commands/cmd_serial.cpp
    commands/cmd_serial_server.cpp
)

add_executable(esphome-cli ${ESPHOME_CLI_SOURCES})

target_include_directories(esphome-cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(esphome-cli
    PRIVATE
        esphome::api
        CLI11::CLI11
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)

if(TARGET esphome_api_message_id)
    add_dependencies(esphome-cli esphome_api_message_id)
endif()

esphome_api_set_warnings(esphome-cli PRIVATE)
# nlohmann/json triggers a false-positive -Wnull-dereference inside its own
# headers under some GCC versions at -O2; keep -Werror everywhere else but don't
# let that third-party-header warning fail the CLI build.
target_compile_options(esphome-cli PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=null-dereference>)
# MSVC flags the standard std::getenv (used to read ESPHOME_CLI_* env vars) as
# C4996 "deprecated/unsafe"; under /WX that fails the build. The call is correct
# and portable, so silence the CRT secure-function deprecation for this target.
target_compile_definitions(esphome-cli PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
esphome_api_enable_sanitizers(esphome-cli)
esphome_api_enable_coverage(esphome-cli)
esphome_api_enable_clang_tidy(esphome-cli)

if(ESPHOME_API_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS esphome-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
