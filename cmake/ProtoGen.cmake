include_guard(GLOBAL)

# Proto pipeline (self-contained — no protoc, no libprotobuf, no abseil):
#
#   1. Build the host tool `esphome_api_protogen` from tools/protogen/*.cpp.
#      It links only the C++ stdlib.
#   2. Run it once on proto/api.proto + proto/api_options.proto to emit, in one
#      shot, into ${ESPHOME_API_GENERATED_DIR}:
#        include/esphome/api/proto/{api_enums.hpp, api_messages.hpp,
#                                   message_id.hpp, api.pb.h}
#        src/{api_messages.cpp, api_registry.cpp}
#   3. Compile the generated sources into the static lib `esphome_api_proto`,
#      together with the committed runtime codec headers (proto/wire.hpp,
#      proto/proto_message.hpp).
#
# Cross-compile note: protogen is a HOST tool. The current CI matrix is not
# cross-compiled, so building it for the host toolchain is correct. A future
# cross build would need a host-built protogen (e.g. via a separate host
# toolchain export); flagged here intentionally.

# ---------------------------------------------------------------------------
# 1. Host generator tool.
# ---------------------------------------------------------------------------
add_executable(esphome_api_protogen
    tools/protogen/lexer.cpp
    tools/protogen/parser.cpp
    tools/protogen/emit_enums.cpp
    tools/protogen/emit_messages.cpp
    tools/protogen/emit_registry.cpp
    tools/protogen/main.cpp
)
target_compile_features(esphome_api_protogen PRIVATE cxx_std_17)
# The generator is a build-time tool; keep our strict warnings off it.

# ---------------------------------------------------------------------------
# 2. Run the generator.
# ---------------------------------------------------------------------------
set(_esphome_api_gen_inc "${ESPHOME_API_GENERATED_DIR}/include/esphome/api/proto")
set(_esphome_api_gen_src "${ESPHOME_API_GENERATED_DIR}/src")

set(_esphome_api_generated_outputs
    "${_esphome_api_gen_inc}/api_enums.hpp"
    "${_esphome_api_gen_inc}/api_messages.hpp"
    "${_esphome_api_gen_inc}/message_id.hpp"
    "${_esphome_api_gen_inc}/api.pb.h"
    "${_esphome_api_gen_src}/api_messages.cpp"
    "${_esphome_api_gen_src}/api_registry.cpp")

add_custom_command(
    OUTPUT ${_esphome_api_generated_outputs}
    COMMAND "$<TARGET_FILE:esphome_api_protogen>"
            "${CMAKE_CURRENT_SOURCE_DIR}/proto/api.proto"
            "${CMAKE_CURRENT_SOURCE_DIR}/proto/api_options.proto"
            "${ESPHOME_API_GENERATED_DIR}"
    DEPENDS esphome_api_protogen
            "${CMAKE_CURRENT_SOURCE_DIR}/proto/api.proto"
            "${CMAKE_CURRENT_SOURCE_DIR}/proto/api_options.proto"
    COMMENT "Generating ESPHome API message classes + registry (protogen)"
    VERBATIM
)
add_custom_target(esphome_api_generate DEPENDS ${_esphome_api_generated_outputs})

# message_id.hpp is consumed by several library TUs; expose a target the library
# can depend on so the generator runs before they compile.
add_custom_target(esphome_api_message_id DEPENDS
    "${_esphome_api_gen_inc}/message_id.hpp")
add_dependencies(esphome_api_message_id esphome_api_generate)

# ---------------------------------------------------------------------------
# 3. Compile the generated sources into the proto static lib.
#
# The generated TU is large and follows protoc's accessor patterns; it is NOT
# subjected to our strict warnings / clang-tidy. Its include dir is exported
# SYSTEM so consumers that pull in api.pb.h do not inherit any warnings either.
# ---------------------------------------------------------------------------
add_library(esphome_api_proto STATIC
    "${_esphome_api_gen_src}/api_messages.cpp"
    "${_esphome_api_gen_src}/api_registry.cpp")
add_dependencies(esphome_api_proto esphome_api_generate)

# Public include: generated/include holds <esphome/api/proto/...> headers; the
# proto-subdir entry resolves the bare `#include "api.pb.h"` in consumer TUs.
target_include_directories(esphome_api_proto SYSTEM PUBLIC
    "${ESPHOME_API_GENERATED_DIR}/include"
    "${_esphome_api_gen_inc}")
# The committed runtime codec (wire.hpp / proto_message.hpp) lives in the normal
# source include tree.
target_include_directories(esphome_api_proto SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(esphome_api_proto PROPERTIES POSITION_INDEPENDENT_CODE ON)
