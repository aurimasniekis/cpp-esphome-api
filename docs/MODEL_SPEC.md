# Typed entity model — implementation spec (for contributors/agents)

You implement fully-typed C++17 wrappers for ESPHome entity domains. Follow the
EXACT pattern of the reference domain.

## Read these first
- `include/esphome/api/model/entities/sensor.hpp` — reference header
- `src/model/entities/sensor.cpp` — reference converter
- `src/model/entity_common.hpp` — `fill_entity_info(info, msg)` helper (copies the 7 common fields)
- `include/esphome/api/model/entity.hpp` — `EntityInfo` base
- `include/esphome/api/model/enums.hpp` — mirrored `enum class`es. USE THESE names. Values equal
  the proto, so convert with `static_cast`.
- `proto/api.proto` — authoritative field definitions (field name = generated accessor, snake_case).

## File layout (per domain `<Camel>`, snake file name `<snake>`)
- Public header: `include/esphome/api/model/entities/<snake>.hpp`  (MUST NOT include api.pb.h)
- Converter:     `src/model/entities/<snake>.cpp`
snake names: binary_sensor, text_sensor, media_player, alarm_control_panel, water_heater, datetime, etc.

## Header contents
Includes: `<esphome/api/model/entity.hpp>`, `<esphome/api/model/enums.hpp>`,
`<esphome/api/proto/proto_fwd.hpp>` (gives `ProtoMessage`), plus `<cstdint> <string> <vector>
<optional> <memory>` as needed.

Forward-declare only the proto messages you reference in signatures:
```cpp
namespace esphome::api { namespace proto {
class ListEntities<Camel>Response;
class <Camel>StateResponse;     // only if it exists
class <Camel>CommandRequest;    // only if it exists
} }
```

1. `struct <Camel>Info : EntityInfo { ... };` — add ONLY the fields beyond the 7 common ones
   already in EntityInfo (key, object_id, name, icon, disabled_by_default, entity_category, device_id).
   - proto enum field -> mirrored `enum class` (same name). `repeated <enum>` -> `std::vector<Enum>`.
     `repeated string` -> `std::vector<std::string>`. repeated nested message -> a small nested
     struct you define + `std::vector<That>`.
2. `struct <Camel>State { std::uint32_t key = 0; ... };` — only if `<Camel>StateResponse` exists.
   Mirror its fields (skip `device_id`). Map enums to mirrored enum class.
3. `struct <Camel>Command { std::uint32_t key = 0; ... };` — only if `<Camel>CommandRequest` exists.
   - CommandRequest fields come in pairs `bool has_x; T x;`. Represent each pair as
     `std::optional<MappedT> x;`. Fields with no `has_` partner are plain members.
   - Declare `std::unique_ptr<ProtoMessage> to_message(const <Camel>Command&);` in the header.

Declare converters in the header:
```cpp
<Camel>Info parse_<snake>_info(const proto::ListEntities<Camel>Response& msg);
<Camel>State parse_<snake>_state(const proto::<Camel>StateResponse& msg);   // if state exists
```

## Converter (.cpp) contents
Include the domain header, `"model/entity_common.hpp"`, and `"api.pb.h"`.
```cpp
<Camel>Info parse_<snake>_info(const proto::ListEntities<Camel>Response& m) {
    <Camel>Info info; fill_entity_info(info, m);
    info.foo = m.foo();
    info.mode = static_cast<SomeEnum>(m.mode());
    for (int i = 0; i < m.modes_size(); ++i) info.modes.push_back(static_cast<SomeEnum>(m.modes(i)));
    return info;
}
<Camel>State parse_<snake>_state(const proto::<Camel>StateResponse& m) { ... }   // if state
std::unique_ptr<ProtoMessage> to_message(const <Camel>Command& c) {
    auto msg = std::make_unique<proto::<Camel>CommandRequest>();
    msg->set_key(c.key);
    if (c.state) { msg->set_has_state(true); msg->set_state(*c.state); }
    if (c.mode)  { msg->set_has_mode(true);  msg->set_mode(static_cast<proto::SomeEnum>(*c.mode)); }
    return msg;
}
```
proto enum C++ type is `esphome::api::proto::<EnumName>`. For a proto field literally named
`has_state`, the generated accessors are `has_state()` / `set_has_state(bool)`.

## Rules
- DO NOT edit CMakeLists.txt, EntityStore, command_builder, client, or files outside your domains.
- DO NOT run cmake/make/build. DO NOT include api.pb.h in any header under include/.
- Verify every field name against proto/api.proto.
