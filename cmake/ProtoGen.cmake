include_guard(GLOBAL)

# Proto pipeline:
#   1. protobuf_generate() compiles proto/api.proto + proto/api_options.proto to
#      C++ into ${ESPHOME_API_GENERATED_DIR}/proto, attached to the static lib
#      `esphome_api_proto`.
#   2. A host tool `esphome_api_gen_message_ids` links that lib and reflects over
#      the descriptors to emit message_id.hpp (the (id) -> name enum).
#
# The generated translation unit is large; it is NOT subjected to our strict
# warnings / clang-tidy. Its include dir is exported SYSTEM so consumers that
# pull in api.pb.h do not inherit protobuf's own warnings either.

find_package(Protobuf REQUIRED)

set(_esphome_api_proto_out "${ESPHOME_API_GENERATED_DIR}/proto")
file(MAKE_DIRECTORY "${_esphome_api_proto_out}")

# Locate the well-known-types directory (for google/protobuf/descriptor.proto).
if(DEFINED Protobuf_INCLUDE_DIRS)
    set(_esphome_api_wkt_dir "${Protobuf_INCLUDE_DIRS}")
elseif(DEFINED protobuf_SOURCE_DIR)
    set(_esphome_api_wkt_dir "${protobuf_SOURCE_DIR}/src")
else()
    get_target_property(_esphome_api_wkt_dir protobuf::libprotobuf
        INTERFACE_INCLUDE_DIRECTORIES)
endif()

add_library(esphome_api_proto STATIC)

# Invoke protoc directly rather than via protobuf_generate(). The protos are
# compiled by basename with `-I proto`, so each file's canonical name is
# "api.proto" / "api_options.proto" — matching the `import "api_options.proto"`
# in api.proto and the `${out}` include directory below. This is deterministic
# across protobuf / CMake versions: some protobuf_generate() variants found in
# clean environments canonicalize relative to the source root instead
# ("proto/api_options.proto"), which mismatches the import and miscompiles the
# generated code.
if(TARGET protobuf::protoc)
    set(_esphome_api_protoc protobuf::protoc)
else()
    set(_esphome_api_protoc "${Protobuf_PROTOC_EXECUTABLE}")
endif()

set(_esphome_api_proto_srcs
    "${_esphome_api_proto_out}/api.pb.cc"
    "${_esphome_api_proto_out}/api_options.pb.cc")

add_custom_command(
    OUTPUT
        ${_esphome_api_proto_srcs}
        "${_esphome_api_proto_out}/api.pb.h"
        "${_esphome_api_proto_out}/api_options.pb.h"
    COMMAND "${_esphome_api_protoc}"
            "--cpp_out=${_esphome_api_proto_out}"
            "-I${CMAKE_CURRENT_SOURCE_DIR}/proto"
            "-I${_esphome_api_wkt_dir}"
            api.proto api_options.proto
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/proto/api.proto"
        "${CMAKE_CURRENT_SOURCE_DIR}/proto/api_options.proto"
        ${_esphome_api_protoc}
    COMMENT "Generating protobuf C++ sources (api.proto, api_options.proto)"
    VERBATIM
)

target_sources(esphome_api_proto PRIVATE ${_esphome_api_proto_srcs})
target_link_libraries(esphome_api_proto PUBLIC protobuf::libprotobuf)
target_include_directories(esphome_api_proto SYSTEM PUBLIC "${_esphome_api_proto_out}")
set_target_properties(esphome_api_proto PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# message_id.hpp generator (host tool)
# ---------------------------------------------------------------------------
add_executable(esphome_api_gen_message_ids tools/gen_message_ids.cpp)
target_link_libraries(esphome_api_gen_message_ids PRIVATE esphome_api_proto)

set(ESPHOME_API_MESSAGE_ID_HPP
    "${ESPHOME_API_GENERATED_DIR}/include/esphome/api/proto/message_id.hpp")

add_custom_command(
    OUTPUT  "${ESPHOME_API_MESSAGE_ID_HPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${ESPHOME_API_GENERATED_DIR}/include/esphome/api/proto"
    COMMAND "$<TARGET_FILE:esphome_api_gen_message_ids>" "${ESPHOME_API_MESSAGE_ID_HPP}"
    DEPENDS esphome_api_gen_message_ids
    COMMENT "Generating esphome/api/proto/message_id.hpp from descriptors"
    VERBATIM
)
add_custom_target(esphome_api_message_id DEPENDS "${ESPHOME_API_MESSAGE_ID_HPP}")
