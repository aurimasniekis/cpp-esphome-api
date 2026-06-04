# ESPHome API Lib

[![CI](https://github.com/aurimasniekis/cpp-esphome-api/actions/workflows/ci.yml/badge.svg)](https://github.com/aurimasniekis/cpp-esphome-api/actions/workflows/ci.yml)
[![Docs](https://github.com/aurimasniekis/cpp-esphome-api/actions/workflows/docs.yml/badge.svg)](https://aurimasniekis.github.io/cpp-esphome-api/)

A fully-typed **C++17 client library for the [ESPHome native API](https://esphome.io/components/api/)** —
the raw TCP + Protocol Buffers protocol that ESPHome devices speak on port `6053`, with optional
**Noise** (`Noise_NNpsk0_25519_ChaChaPoly_SHA256`) encryption.

It gives you an object-oriented, typed surface over every ESPHome subsystem (sensors, lights, covers,
climate, locks, media players, the Bluetooth/voice/Z-Wave/serial proxies, logs, …) while still letting
you reach the raw generated protobuf messages when you need an escape hatch.

The repository also ships a full command-line tool, **`esphome-cli`**, built on top of this library.
Its documentation lives in **[`bin/README.md`](bin/README.md)**.

---

## Why use this library?

ESPHome devices expose a binary protobuf protocol. Talking to it by hand means framing messages,
running a Noise handshake, mapping message IDs to types, and decoding 26 entity domains. This library
does all of that and hands you typed objects.

- **Good for** desktop/server applications that need to read state from, or control, ESPHome devices
  directly — without going through Home Assistant.
- **Good for** writing automation, dashboards, bridges, or test harnesses in C++.
- **Avoids** hand-rolling protobuf framing, the Noise handshake, and message dispatch.
- **Useful when** you want a typed API (`light->set_brightness(0.5F)`) instead of raw protobuf.
- **Useful when** you want a blocking, request/response style (`SyncClient`) *or* an async event loop
  (`Client`) — both are provided.
- **Not ideal for** running *on* an ESP32 itself (this is a host-side client), or for environments
  without C++17 and CMake ≥ 3.25.

---

## Quick example

The smallest useful program: connect, print the device identity, read sensors, toggle a light.

```cpp
#include <esphome/api/api.hpp>

#include <iostream>

int main() {
    esphome::api::ClientOptions options;
    options.host = "living-room.local";
    // For an encrypted device, set the base64 32-byte pre-shared key:
    // options.connection.noise_psk = "your-base64-psk";

    try {
        esphome::api::SyncClient client(options);
        client.connect();  // TCP + handshake, then auto-enumerate entities and subscribe to states

        const esphome::api::DeviceInfo info = client.device_info();
        std::cout << info.name << " — " << info.model
                  << " (ESPHome " << info.esphome_version << ")\n";

        // Typed, object-oriented entity access.
        for (auto sensor : client.entities().sensors())
            std::cout << sensor.name() << " = " << sensor.value() << "\n";

        if (auto light = client.entities().light("ceiling")) {
            light->turn_on();
            light->set_brightness(0.5F);
        }

        client.disconnect();
    } catch (const esphome::api::ApiError& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
```

**Why this works:**

- `SyncClient` is a *blocking* facade. `connect()` performs the TCP connect and protocol handshake,
  then — because `ClientOptions::subscribe_on_connect` defaults to `true` — it also enumerates the
  device's entities and subscribes to state updates. So `entities()` is already populated and live
  when `connect()` returns.
- `client.entities().light("ceiling")` returns a `std::optional<LightEntity>`. It is empty if the
  device has no light with that object id, which is why the example checks it with `if (auto …)`.
- Every failure (connect refused, bad key, timeout, …) is reported by throwing a subclass of
  `esphome::api::ApiError`. Catch the base class to handle them all.

---

## Installation

The library is a normal CMake project. The exported target is **`esphome::api`**.

### CMake `FetchContent` (recommended)

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    esphome-api
    URL      https://github.com/aurimasniekis/cpp-esphome-api/archive/refs/tags/v0.1.0.tar.gz
    URL_HASH SHA256=0000000000000000000000000000000000000000000000000000000000000000
)
FetchContent_MakeAvailable(esphome-api)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE esphome::api)
```

When the library is consumed this way, its tests, examples, and the `esphome-cli` tool are **off by
default** (they only build when the library is the top-level project), so you pay only for the library
itself.

### CMake `add_subdirectory`

If you vendor the sources (e.g. as a git submodule):

```cmake
add_subdirectory(third_party/cpp-esphome-api)
target_link_libraries(my_app PRIVATE esphome::api)
```

### Installed package

After `make install` (see [Requirements](#requirements) for the caveat), consumers can use the
installed CMake package config:

```cmake
find_package(esphome-api-client CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE esphome::api)
```

> The package is only installable/exportable when its dependencies (Protocol Buffers / Abseil /
> libsodium) come from the system. If any of them had to be fetched and built from source, install
> rules are disabled automatically, because a source-built dependency cannot be re-exported. Provide
> a system protobuf + libsodium to enable `make install`.

---

## Requirements

- **C++ standard:** C++17 (`cxx_std_17` is a public compile feature of the target).
- **Build system:** CMake ≥ 3.25.
- **Threads:** the system threads library (linked publicly).
- **Protocol Buffers:** used from the system if available, otherwise built from source as part of the
  build.
- **Asio** and **libsodium:** fetched automatically via CMake `FetchContent`. They are *private*
  dependencies — they never appear in the public headers, so consumers do not need to find or link
  them.
- **Encryption (optional):** Noise support is controlled by `ESPHOME_API_WITH_NOISE` (default `ON`).
  The public macro `ESPHOME_API_HAS_NOISE` reflects whether it was compiled in, so `config.hpp` and
  your own code can branch on the feature.

---

## Core concepts

### `ClientOptions`

Everything needed to reach one device. Plain struct — fill in what you need.

```cpp
esphome::api::ClientOptions options;
options.host = "kitchen.local";          // hostname or IP
options.port = 6053;                     // default
options.subscribe_on_connect = true;     // default: auto-enumerate + subscribe on connect()
options.connection.noise_psk = "...";    // base64 32-byte key; empty ⇒ plaintext
options.connection.expected_name = "kitchen";  // optional handshake guard
```

`ClientOptions::connection` is a `ConnectionOptions` with handshake/keepalive tunables:
`client_info`, `noise_psk`, `expected_name`, `login` (send an `AuthenticationRequest`, default on),
`password` (deprecated plaintext auth), and the `connect_timeout` / `keepalive_interval` /
`keepalive_timeout` durations.

### `SyncClient`

The blocking facade. Each call pumps the internal Asio event loop until the request resolves (or its
timeout fires). Use it for scripts and request/response code.

```cpp
esphome::api::SyncClient client(options);
client.set_request_timeout(std::chrono::seconds(10));  // optional
client.connect();
// ... use the client ...
client.disconnect();
```

Key members: `connect()`, `disconnect()`, `is_connected()`, `state()`, `server_hello()`,
`device_info()`, `entities()`, `store()`, the subsystem accessors below, `list_entities()`,
`subscribe_states()`, and `pump_until(predicate, timeout)` for waiting on asynchronous results.

### `Client` (async)

The async core. You drive the Asio event loop yourself and react to callbacks. Use it when you have
your own event loop or need many concurrent connections.

```cpp
esphome::api::Client client(options);
client.on_state([](esphome::api::ConnectionState s) { /* connected, disconnected, ... */ });
client.async_connect([&](std::error_code ec) {
    if (!ec) client.subscribe_to_states();
});
client.run();  // or run_for(...) / poll() / run_one()
```

`SyncClient::async()` exposes the underlying `Client` if you started sync and need the loop.

### `EntityRegistry` — typed entity access

`client.entities()` returns an `EntityRegistry`. Plural accessors return iterable lists; singular
accessors take an object id (or numeric key) and return a `std::optional<…Entity>`.

```cpp
auto reg = client.entities();

for (auto s : reg.sensors())
    std::cout << s.name() << " = " << s.value() << "\n";

if (auto sw = reg.switch_("relay"))   // note the trailing underscore: 'switch' is a keyword
    sw->toggle();

if (auto cover = reg.cover("garage"))
    cover->open();
```

Handles are lightweight views into the client's entity store. They are valid while the client is
connected and the store holds the entity; do not keep them past `disconnect()`.

### `EntityStore` — the live state cache

`client.store()` is the lower-level cache the registry sits on. Use it to iterate everything or to get
a callback on each state change.

```cpp
client.store().on_state([&](esphome::api::EntityType type, std::uint32_t key) {
    if (const auto* e = client.store().find(key))
        std::cout << e->object_id << " changed\n";
});
client.subscribe_states();
```

### Subsystems

Reachable directly from the client; each is a typed manager.

```cpp
client.logs().subscribe(esphome::api::LogLevel::Debug,
    [](const esphome::api::LogEntry& e) { std::cout << e.message << "\n"; });

client.bluetooth().subscribe_advertisements(
    [](const esphome::api::BleAdvertisement& adv) { /* adv.address, adv.rssi, adv.name */ });
```

Other accessors: `home_assistant()`, `voice()`, `zwave()`, `serial()`, and `serial_proxy(index)` /
`serial_proxy(name)` for a single UART port handle.

### `Discovery`

Find devices on the LAN over mDNS — no connection required.

```cpp
#include <esphome/api/discovery.hpp>

for (const auto& d : esphome::api::Discovery::scan(std::chrono::seconds(2)))
    std::cout << d.name << "  " << d.connect_host() << ":" << d.port
              << (d.requires_encryption ? "  (encryption required)" : "") << "\n";
```

`DiscoveredDevice::connect_host()` returns the resolved IP if known, otherwise the hostname. Scanning
requires multicast access to `224.0.0.251:5353`.

---

## Common usage patterns

### Connect to an encrypted device

```cpp
esphome::api::ClientOptions options;
options.host = "bedroom.local";
options.connection.noise_psk = "base64-encoded-32-byte-key";

esphome::api::SyncClient client(options);
client.connect();  // throws EncryptionMismatchError if the device is actually plaintext
```

The transport (plaintext vs Noise) is selected automatically from the device's indicator byte; you
only supply the key. Connecting with a key to a plaintext-only device — or without one to a device
that requires Noise — throws `EncryptionMismatchError`, whose message names the fix.

### Guard against connecting to the wrong host

```cpp
options.connection.expected_name = "bedroom";  // handshake fails if the device reports another name
```

### Control entities by domain

```cpp
if (auto n = client.entities().number("setpoint")) n->set(21.5F);
if (auto sel = client.entities().select("mode"))   sel->set("eco");
if (auto btn = client.entities().button("restart")) btn->press();
if (auto lock = client.entities().lock("front"))   lock->unlock();
```

Each returns `std::optional`, so a missing entity is a normal, checkable result — not an exception.

### Wait for an asynchronous result with `SyncClient`

Some operations (a Bluetooth GATT read, a serial modem-pin query) complete via callback. Drive the
loop until your predicate is satisfied:

```cpp
bool done = false;
esphome::api::BleGattReadResult result;
client.bluetooth().read(addr, handle, [&](const auto& r) { result = r; done = true; });

try {
    client.pump_until([&] { return done; }, std::chrono::seconds(5));
} catch (const esphome::api::TimeoutError&) {
    std::cerr << "GATT read timed out\n";
}
```

`pump_until` throws `TimeoutError` if the deadline passes before the predicate becomes true.

### Optional JSON (de)serialization

If your project already uses [nlohmann/json](https://github.com/nlohmann/json), include the
header-only adapter to convert any value type to/from JSON:

```cpp
#include <esphome/api/json.hpp>   // guarded by __has_include; NOT pulled in by <esphome/api/api.hpp>

nlohmann::json j = client.device_info();
```

This is intentionally separate so the core library stays JSON-free for consumers who do not want the
dependency.

---

## Error handling

The library reports errors by **throwing exceptions**. All derive from `esphome::api::ApiError`
(itself a `std::runtime_error`), so catching the base class catches everything.

| Exception                 | Meaning                                                                                                                                                                 |
|---------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `ApiError`                | Base class for every library error.                                                                                                                                     |
| `ConnectionError`         | TCP-level failure: refused, reset, EOF, host unreachable.                                                                                                               |
| `ProtocolError`           | Peer violated framing/message protocol (bad indicator byte, oversized frame, undecodable payload).                                                                      |
| `HandshakeError`          | Noise handshake failed (bad PSK, MAC mismatch, unexpected message). *Subclass of `ProtocolError`.*                                                                      |
| `EncryptionMismatchError` | Encryption mode does not match the device (key supplied to a plaintext device, or missing for an encrypted one). *Subclass of `HandshakeError`; message names the fix.* |
| `AuthenticationError`     | Auth rejected, or negotiated API version unsupported.                                                                                                                   |
| `TimeoutError`            | An operation did not complete before its deadline.                                                                                                                      |
| `EncryptionError`         | A symmetric encrypt/decrypt failed (AEAD tag mismatch, bad nonce). *Subclass of `ProtocolError`.*                                                                       |

```cpp
try {
    client.connect();
} catch (const esphome::api::EncryptionMismatchError& e) {
    std::cerr << e.what() << "\n";        // already tells you to add or drop the PSK
} catch (const esphome::api::TimeoutError&) {
    std::cerr << "device did not respond in time\n";
} catch (const esphome::api::ApiError& e) {
    std::cerr << "connect failed: " << e.what() << "\n";
}
```

Note that entity lookups (`entities().light("x")`) do **not** throw for a missing entity — they return
an empty `std::optional`. Exceptions are reserved for connection, protocol, and timeout failures.

---

## Edge cases and pitfalls

- **`switch_` has a trailing underscore.** `switch` is a C++ keyword, so the registry accessor is
  `entities().switch_(...)`. Some struct names are similarly disambiguated (`LockEntityState`,
  `MediaPlayerStatus`, `AlarmControlPanelStatus`, `UpdateControl`).
- **Entities are empty until you connect.** With the default `subscribe_on_connect = true`,
  `connect()` populates them. If you set it to `false`, call `list_entities()` (and
  `subscribe_states()` for live updates) yourself, then pump the loop before reading.
- **Handles are views, not owners.** A `LightEntity` / `SwitchEntity` etc. references state owned by
  the client's store. Using one after `disconnect()` (or after the client is destroyed) is a
  use-after-free. Re-fetch from `entities()` instead of caching handles long-term.
- **State after a command is not instantaneous.** A device reports the new state asynchronously. After
  issuing a command, `pump_until(..., timeout)` briefly to let the state update arrive before reading
  it back.
- **A missing entity is `std::optional{}`, not an exception.** Always check the optional.
- **`Discovery::scan` needs multicast.** It requires access to `224.0.0.251:5353`; firewalled or
  container networks without multicast will return nothing.
- **Keys are sensitive.** A Noise PSK is a 32-byte secret. Treat it like a password; do not log it.

---

## API overview

| API                                                                                | Purpose                      | Notes                                                |
|------------------------------------------------------------------------------------|------------------------------|------------------------------------------------------|
| `ClientOptions`                                                                    | Connection parameters        | `host`, `port`, `connection`, `subscribe_on_connect` |
| `ConnectionOptions`                                                                | Handshake/keepalive tunables | `noise_psk`, `expected_name`, `login`, timeouts      |
| `SyncClient`                                                                       | Blocking client              | Pumps the loop per call; throws on failure           |
| `Client`                                                                           | Async client                 | You drive the Asio loop (`run` / `run_for` / `poll`) |
| `EntityRegistry` (`entities()`)                                                    | Typed entity access          | Plural lists; singular returns `std::optional`       |
| `EntityStore` (`store()`)                                                          | Live state cache             | `entities()`, `find(key)`, `on_state(cb)`            |
| `DeviceInfo` (`device_info()`)                                                     | Device identity              | name, model, version, MAC, project, …                |
| `bluetooth()` / `logs()` / `serial()` / `voice()` / `zwave()` / `home_assistant()` | Subsystem managers           | Typed proxies                                        |
| `Discovery::scan(timeout)`                                                         | mDNS LAN discovery           | Returns `std::vector<DiscoveredDevice>`              |
| `ApiError` and subclasses                                                          | Error reporting              | See [Error handling](#error-handling)                |
| `<esphome/api/json.hpp>`                                                           | Optional JSON adapter        | Requires nlohmann/json; not auto-included            |

For the full surface, see the headers under `include/esphome/api/` (or build the Doxygen docs with
`make docs`).

---

## Examples

The `examples/` directory contains runnable programs. Each accepts `<host> [port]` plus
`--key|-k <base64-psk>` (for Noise) and `--name|-n <device-name>`.

| Example                         | Demonstrates                                                  |
|---------------------------------|---------------------------------------------------------------|
| `examples/hello.cpp`            | Connect, print device identity, disconnect — the minimal flow |
| `examples/list_entities.cpp`    | Enumerate and print every entity by domain                    |
| `examples/subscribe_states.cpp` | Stream live state changes                                     |
| `examples/control_light.cpp`    | Find the first light and toggle it via the typed handle       |
| `examples/logs.cpp`             | Stream the device log to stdout                               |
| `examples/bluetooth_scan.cpp`   | Subscribe to BLE advertisements via the device's proxy        |
| `examples/noise_connect.cpp`    | Encrypted connect (requires `--key`)                          |
| `examples/discover.cpp`         | Scan the LAN for ESPHome devices over mDNS                    |

Build them with `make examples`, then run e.g.:

```sh
./build/debug/examples/esphome_api_hello living-room.local
./build/debug/examples/esphome_api_control_light bedroom.local --key <base64-psk>
```

---

## Testing

```sh
make test         # configure + build + run unit/integration tests (Debug)
make release      # optimized build + tests
make sanitize     # ASan + UBSan build + tests
make coverage     # Clang source-based coverage (HTML)
make ci           # format-check + clang-tidy + tests + sanitizers + release  (pre-push gate)
```

Or directly with CMake/CTest:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The test suite includes an in-process device emulator (plaintext + Noise, over a real socket), so the
integration tests exercise the full stack. The Noise handshake is verified byte-for-byte against the
official `NNpsk0_25519_ChaChaPoly_SHA256` test vector.

CMake options use the `ESPHOME_API_` prefix: `WITH_NOISE` (default ON), `BUILD_TESTS`,
`BUILD_EXAMPLES`, `BUILD_CLI`, `BUILD_SHARED`, `BUILD_DOCS`, `ENABLE_SANITIZERS`, `ENABLE_COVERAGE`,
`ENABLE_CLANG_TIDY`, `WARNINGS_AS_ERRORS`, `INSTALL`.

---

## Architecture

Seven layers, each depending only on those below:

1. **Transport + crypto** — Asio `TcpTransport`; pure libsodium-backed Noise primitives / handshake.
2. **Frame layer** — `PlaintextFrameHelper` / `NoiseFrameHelper` (reassembly + framing).
3. **Codec + registry** — `MessageRegistry` maps message id ↔ protobuf type via runtime reflection.
4. **Connection** — explicit state machine: handshake, dispatch, keepalive.
5. **Typed model** — `Client`, `DeviceInfo`, `EntityStore`, per-domain entities + commands.
6. **Subsystem managers** — logs, HA services, Bluetooth/voice/Z-Wave/serial proxies.
7. **Sync facade** — `SyncClient` pumps the event loop until each blocking call resolves.

The protobuf schema (`proto/api.proto`) is vendored from ESPHome and compiled at build time; a small
C++ host tool generates the `MessageId` enum from the descriptors (no Python in the build).

---

## FAQ

**Do I need to link Asio or libsodium myself?**
No. They are private dependencies hidden behind the public API. Link only `esphome::api`.

**Is it header-only?**
No. It is a compiled static (or shared, with `ESPHOME_API_BUILD_SHARED`) library.

**Sync or async?**
Both. Start with `SyncClient` for scripts; use `Client` when you have your own event loop.

**What happens if an entity doesn't exist?**
The singular accessor returns an empty `std::optional`. No exception.

**Can I use the raw protobuf messages?**
Yes — `client.send(msg)`, `client.on(id, ...)`, and `client.on_raw(...)` expose the generated
`esphome::api::proto::*` types as an escape hatch.

**How do I enable encryption?**
Set `options.connection.noise_psk` to the device's base64 32-byte key. The transport is chosen
automatically.

**Where is the command-line tool documented?**
In [`bin/README.md`](bin/README.md).

---

## Command-line tool

`esphome-cli` wraps this library for shell use: discover devices, inspect entities, control any
domain, stream logs and state, and drive the Bluetooth and serial proxies.

```sh
esphome-cli scan
esphome-cli living-room.local info --entities
esphome-cli living-room light main turn_on 50%
esphome-cli bedroom logs --level debug
```

Install it via Docker or Homebrew, and read the full reference here:

- **Docker:** `docker run --rm --network host aurimasniekis/esphome-cli scan`
- **Homebrew:** `brew install aurimasniekis/tap/esphome-cli`
- **Full docs:** **[`bin/README.md`](bin/README.md)**

The CLI's own dependencies (CLI11, spdlog, nlohmann/json) are private to the tool and never reach the
library target.

## Contributing

Contributions to the library are welcome! If you encounter any issues or have suggestions for
improvements,
please feel free to submit a pull request or open an issue on the project's repository.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
