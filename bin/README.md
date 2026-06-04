# esphome-cli

[![CI](https://github.com/aurimasniekis/cpp-esphome-api/actions/workflows/ci.yml/badge.svg)](https://github.com/aurimasniekis/cpp-esphome-api/actions/workflows/ci.yml)
[![Docs](https://github.com/aurimasniekis/cpp-esphome-api/actions/workflows/docs.yml/badge.svg)](https://aurimasniekis.github.io/cpp-esphome-api/)

A command-line tool to **discover, inspect, and control [ESPHome](https://esphome.io) devices** over
their native API (TCP + protobuf on port `6053`, optional Noise encryption). It is built on top of the
[ESPHome API Lib](../README.md) and speaks to devices directly — no Home Assistant required.

With one binary you can:

- Find devices on the LAN over mDNS (`scan`).
- Show device info and list every entity with its state and available actions.
- Control any entity domain — lights, switches, covers, climate, locks, media players, and more.
- Stream device logs and live entity state changes.
- Drive the device's **Bluetooth proxy** (scan / connect / GATT read/write / service discovery).
- Drive the device's **serial (UART) proxy**, including a PTY bridge that exposes a remote UART as a
  local terminal device.
- Remember per-device addresses, ports, and encryption keys in a config file.

Output can be human-readable **text** (default), **JSON**, or **YAML**.

---

## Installation

### Homebrew

```sh
brew install aurimasniekis/tap/esphome-cli
```

### Linux packages (.deb / .rpm)

Each [release](https://github.com/aurimasniekis/cpp-esphome-api/releases) attaches `.deb` and `.rpm`
packages for `x86_64` and `arm64`. Download the one for your distro and architecture, then:

```sh
# Debian / Ubuntu
sudo apt install ./esphome-cli_0.3.0_amd64.deb

# Fedora / RHEL / openSUSE
sudo rpm -i esphome-cli-0.3.0-1.x86_64.rpm
```

The package installs `esphome-cli` to `/usr/bin`.

### Docker

The image is published as `aurimasniekis/esphome-cli`. Because discovery and device traffic need LAN
access (including mDNS multicast), run with host networking:

```sh
docker run --rm --network host aurimasniekis/esphome-cli scan
docker run --rm --network host aurimasniekis/esphome-cli living-room.local info
```

To persist the config file across runs, mount a directory and point the CLI at it:

```sh
docker run --rm --network host \
  -v "$HOME/.config/esphome-cli:/config" \
  -e ESPHOME_CLI_CONFIG=/config/config.json \
  aurimasniekis/esphome-cli devices
```

> `scan` (mDNS) and BLE/serial streaming rely on multicast and a direct route to the device. Host
> networking is the simplest way to get that inside Docker.

### Build from source

The CLI is part of the library repository and builds by default at the top level:

```sh
make cli            # → build/bin/esphome-cli
# or, with CMake directly:
cmake -S . -B build -DESPHOME_API_BUILD_CLI=ON
cmake --build build --target esphome-cli
```

---

## Command model

There are two shapes of invocation:

```
esphome-cli [globals] <command> [args]              # device-independent commands
esphome-cli [globals] <host> <subcommand> [args]    # commands against one device
```

Global commands (no host): `scan`, `config`, `devices`, `help`.

Host subcommands (after a `<host>`): `info`, `list`, `<domain> …`, `logs`, `watch`, `bt`,
`serial-proxy`.

`<host>` may be a hostname, an IP, or a saved-device alias from your config. If you set the
`ESPHOME_CLI_HOST` environment variable, you can omit the host and start directly with a host
subcommand (e.g. `esphome-cli info`).

Run `esphome-cli` with no arguments, or `esphome-cli help`, to print the root help. Use
`esphome-cli help <topic>` for contextual help, e.g. `help light`, `help bt`, `help serial-proxy`.

---

## Global options

These apply to any command. **Precedence: command-line flag > environment variable > config file >
built-in default.**

| Flag                             | Env var                 | Meaning                                |
|----------------------------------|-------------------------|----------------------------------------|
| `--output text\|json\|yaml`      | `ESPHOME_CLI_OUTPUT`    | Output format (default `text`)         |
| `--log-level LEVEL`              | `ESPHOME_CLI_LOG_LEVEL` | CLI log verbosity (to stderr)          |
| `--config PATH`                  | `ESPHOME_CLI_CONFIG`    | Config file location                   |
| `-p, --port N`                   | `ESPHOME_CLI_PORT`      | API TCP port (default `6053`)          |
| `-k, --key PSK`                  | `ESPHOME_CLI_KEY`       | Base64 Noise PSK (enables encryption)  |
| `-n, --name NAME`                | `ESPHOME_CLI_NAME`      | Expected device name (handshake guard) |
| `--save-keys` / `--no-save-keys` | `ESPHOME_CLI_SAVE_KEYS` | Persist encryption keys to config      |
| `--timeout MS`                   | `ESPHOME_CLI_TIMEOUT`   | Per-request timeout in milliseconds    |
| (host)                           | `ESPHOME_CLI_HOST`      | Default device when no host is given   |

Valid `--log-level` values: `trace`, `debug`, `info`, `warn`/`warning`, `error`/`err`, `critical`,
`off` (default `warn`). Valid `--output` values: `text`, `json`, `yaml`/`yml`.

Logs go to **stderr**; command results go to **stdout** — so you can pipe JSON/YAML cleanly while
still seeing diagnostics.

---

## Global commands

### `scan` — discover devices on the LAN

```sh
esphome-cli scan            # scan for the configured default duration
esphome-cli scan 5          # scan for 5 seconds
```

Uses mDNS (`_esphomelib._tcp`). Prints each device's name, host/port, MAC, ESPHome version, and
whether encryption is supported or required. With `--output json`/`yaml`, emits a structured array.

> Requires multicast access to `224.0.0.251:5353`. In Docker, use `--network host`.

### `config` — manage saved devices and defaults

```sh
esphome-cli config list                       # show defaults + saved devices (default action)
esphome-cli config get living-room            # show one saved device
esphome-cli config path                       # print the config file path
esphome-cli config forget living-room         # remove a saved device

# Save per-device fields (address/port/psk/name/expected_name/timeout_ms):
esphome-cli config set living-room psk=<base64> port=6053 expected_name=living-room

# Change global defaults (output/log_level/scan_timeout_ms/timeout_ms/save_keys):
esphome-cli config set defaults output=json timeout_ms=10000 save_keys=true
```

`config set <host> …` always stores the key, regardless of the `--save-keys` setting (you asked for
it explicitly). The first time a key is written, the tool prints a one-time note that keys are stored
in plaintext (the file is mode `0600`); use `--no-save-keys` to avoid persisting keys.

### `devices` — list saved devices

```sh
esphome-cli devices
```

A compact listing of everything in your config: alias, friendly name, address:port, whether a key is
stored, and any expected-name guard.

### `help` — contextual help

```sh
esphome-cli help                 # root help
esphome-cli help light           # actions for the light domain
esphome-cli help bt              # Bluetooth proxy subcommands
esphome-cli help serial-proxy    # serial proxy subcommands
esphome-cli help config          # config actions and keys
```

---

## Host subcommands

All of these connect to `<host>`. A successful connect persists/updates the device record in your
config (address, port, name, expected-name, and the key if `--save-keys` is in effect).

### `info` — device identity (and optionally entities)

```sh
esphome-cli living-room info
esphome-cli living-room info --entities      # also list every entity with its info + state
```

### `list` — every entity with info, state, and actions

```sh
esphome-cli kitchen list
```

Lists all entities, each annotated with its current state and the actions you can run on it.

### `<domain> …` — inspect and control entities

This is the core control surface. The general form is:

```
esphome-cli <host> <domain>                          # list entities in the domain
esphome-cli <host> <domain> <object-id>              # describe one entity (info + state + actions)
esphome-cli <host> <domain> <object-id> state        # just the current state
esphome-cli <host> <domain> <object-id> info         # just the static info
esphome-cli <host> <domain> <object-id> <action> [values...]   # perform an action
```

`state` and `info` are available on **every** entity. Other actions depend on the domain (see the
table below). After an action runs, the CLI pumps briefly for the resulting state change and prints
the entity's new state.

Examples:

```sh
esphome-cli living-room light                        # list lights
esphome-cli living-room light main                   # describe the 'main' light
esphome-cli living-room light main turn_on           # turn on
esphome-cli living-room light main turn_on 50%       # turn on at 50% brightness
esphome-cli living-room light main set_brightness 0.5
esphome-cli living-room light main set_rgb 255,0,0   # or: set_rgb 255 0 0
esphome-cli living-room light main command rgb=255,128,0 brightness=80% transition=2

esphome-cli kitchen switch relay toggle
esphome-cli kitchen switch relay set on
esphome-cli kitchen number setpoint set 21.5
esphome-cli kitchen select mode set eco
esphome-cli kitchen button restart press

esphome-cli den cover garage open
esphome-cli den cover garage set_position 50%
esphome-cli den climate hvac set_mode heat
esphome-cli den climate hvac set_target_temperature 22
esphome-cli den climate hvac command mode=cool temperature=20 humidity=45

esphome-cli front lock door unlock
esphome-cli media room media_player speaker set_volume 30%
esphome-cli alarm panel main arm_away 1234           # optional code as the value
esphome-cli clock date today set 2026-06-04
esphome-cli clock time alarm set 07:30
esphome-cli sensors update firmware install
```

#### Domains and their actions

`state` and `info` are implicit on every entity. Value formats: `50%` or `0.5` for fractions; RGB as
`r,g,b` or three numbers (0–255 or 0–1); booleans accept `on/off`, `true/false`, `yes/no`, `1/0`.

| Domain                | Actions                                                                                                |
|-----------------------|--------------------------------------------------------------------------------------------------------|
| `switch`              | `turn_on`, `turn_off`, `toggle`, `set <bool>`                                                          |
| `light`               | `turn_on [level]`, `turn_off`, `toggle`, `set_brightness <level>`, `set_rgb <r,g,b>`, `command <k=v…>` |
| `fan`                 | `turn_on`, `turn_off`, `toggle`, `set_speed <n>`                                                       |
| `cover`               | `open`, `close`, `stop`, `set_position <level>`                                                        |
| `valve`               | `open`, `close`, `stop`, `set_position <level>`                                                        |
| `climate`             | `set_mode <mode>`, `set_target_temperature <°>`, `command <k=v…>`                                      |
| `number`              | `set <value>`                                                                                          |
| `select`              | `set <option>`                                                                                         |
| `text`                | `set <value>`                                                                                          |
| `button`              | `press`                                                                                                |
| `lock`                | `lock`, `unlock`, `open`                                                                               |
| `media_player`        | `play`, `pause`, `stop`, `set_volume <level>`                                                          |
| `siren`               | `turn_on`, `turn_off`, `toggle`                                                                        |
| `alarm_control_panel` | `arm_away [code]`, `arm_home [code]`, `disarm [code]`                                                  |
| `date`                | `set <YYYY-MM-DD>`                                                                                     |
| `time`                | `set <HH:MM[:SS]>`                                                                                     |
| `datetime`            | `set <epoch-seconds>`                                                                                  |
| `update`              | `install`, `check`                                                                                     |
| `camera`              | `request_image`                                                                                        |

Read-only domains (`binary_sensor`, `sensor`, `text_sensor`, `event`, `water_heater`, `infrared`,
`radio_frequency`) support listing and the `state` / `info` pseudo-actions but have no control verbs.

The `light command` and `climate command` actions take `key=value` tokens for fine control. Light
fields: `state`, `brightness`, `color_brightness`, `rgb`, `white`, `color_temperature`, `cold_white`,
`warm_white`, `transition`(`_length`), `flash`(`_length`), `effect`. Climate fields: `mode`,
`target_temperature` (alias `temperature`), `target_temperature_low`, `target_temperature_high`,
`target_humidity` (alias `humidity`).

Climate modes: `off`, `heat`, `cool`, `heat_cool` (aliases `heatcool`/`auto_heat_cool`), `auto`,
`fan_only` (alias `fan`), `dry`.

### `logs` — stream device logs

```sh
esphome-cli bedroom logs                 # default level (info)
esphome-cli bedroom logs --level debug   # minimum level
```

Streams until interrupted (Ctrl-C). Levels: `none`, `error`/`err`, `warn`/`warning`, `info`,
`config`, `debug`, `verbose`, `very_verbose`.

### `watch` — stream entity state changes

```sh
esphome-cli living-room watch                 # watch everything
esphome-cli living-room watch 'light.*'       # only lights
esphome-cli living-room watch 'sensor.temp*' 'switch.relay'
```

Patterns are globs (`*` and `?`) over `<domain>.<object_id>`; multiple patterns are OR-ed; no pattern
watches everything. Each change prints a timestamped line (or a JSON/YAML object per line in those
output modes). Runs until Ctrl-C.

### `bt` — Bluetooth proxy

Drive a device that acts as a BLE proxy. Addresses are MACs (`AA:BB:CC:DD:EE:FF`), or hex (`0x…`) /
decimal numbers.

```sh
esphome-cli hub bt scan                          # stream advertisements until Ctrl-C
esphome-cli hub bt scan 10                        # scan for 10 seconds, then stop
esphome-cli hub bt scan active                    # active scanning
esphome-cli hub bt connect AA:BB:CC:DD:EE:FF
esphome-cli hub bt services AA:BB:CC:DD:EE:FF      # discover the GATT service table
esphome-cli hub bt read AA:BB:CC:DD:EE:FF 42       # read GATT handle 42 (prints hex)
esphome-cli hub bt write AA:BB:CC:DD:EE:FF 42 0a1b # write hex bytes (with response)
esphome-cli hub bt write AA:BB:CC:DD:EE:FF 42 0a1b no-response
```

Write data is hex (whitespace ignored; must be an even number of digits). `no-response` (alias
`noresp`) issues a write-without-response.

### `serial-proxy` — serial (UART) proxy

Interact with a UART exposed by the device's serial proxy. The port can be given by numeric index
(default `0`) or by its advertised name.

```sh
esphome-cli gw serial-proxy listen                # dump incoming bytes (Ctrl-C to stop)
esphome-cli gw serial-proxy listen 1              # port index 1

esphome-cli gw serial-proxy configure baud=115200 parity=none stop=1 data=8
esphome-cli gw serial-proxy configure 1 baud=9600 flow=off

esphome-cli gw serial-proxy write 0 "AT\r\n"      # write to a port
esphome-cli gw serial-proxy pins get              # read RTS/DTR modem lines
esphome-cli gw serial-proxy pins set rts=1 dtr=0

# Bridge a remote UART to a local PTY (POSIX only):
esphome-cli gw serial-proxy server --link /tmp/esphome-tty --baud 115200
```

`configure` keys: `baud`(`rate`), `parity` (`none`/`even`/`odd`, or `n`/`e`/`o`), `stop`(`_bits`),
`data`(`_size`/`_bits`), `flow`(`_control`, boolean).

#### `server` — PTY bridge (POSIX only)

`serial-proxy server` opens a local pseudo-terminal, symlinks it to the path given by `--link`
(default `/tmp/esphome-tty.<index>`), configures the remote UART, and bridges bytes both ways until
Ctrl-C. Point any serial program (e.g. `minicom`, `screen`, `picocom`) at the link path to talk to the
remote UART as if it were local.

Options: `--link PATH`, `--baud N`, `--stop N`, `--data N`, `--rts 0|1`, `--dtr 0|1`. On Windows this
subcommand is unavailable and exits with an error.

---

## Output formats

```sh
esphome-cli living-room info                 # human-readable text (default)
esphome-cli living-room info --output json   # pretty JSON
esphome-cli living-room info --output yaml    # YAML (built-in emitter)
```

Streaming commands (`logs`, `watch`, `bt scan`, `serial-proxy listen`) print one record per line:
plain text by default, a compact JSON object per line under `--output json`, or a YAML list item under
`--output yaml`. This makes them easy to pipe into `jq` or a log processor.

---

## Configuration file

- **Location:** `$XDG_CONFIG_HOME/esphome-cli/config.json`, falling back to
  `~/.config/esphome-cli/config.json` (or `./.esphome-cli/config.json` if neither `XDG_CONFIG_HOME`
  nor `HOME` is set). Override with `--config` / `ESPHOME_CLI_CONFIG`.
- **Permissions:** the directory is created `0700` and the file `0600`.
- **Contents:** global defaults (`output`, `log_level`, `scan_timeout_ms`, `timeout_ms`, `save_keys`)
  and a map of saved devices (address, port, optional PSK, name, expected-name, per-device timeout).
- **Auto-save:** a successful connect updates the matching device record. The key is stored only when
  key-saving is enabled (default on; disable with `--no-save-keys` or `save_keys=false`).
- **Lookups:** a host argument is matched against a saved record by alias, then by address, then by
  device name — so `esphome-cli living-room …` works once `living-room` is saved.

A corrupt config file is ignored (the tool falls back to defaults rather than overwriting it).

---

## Exit codes

| Code | Meaning                                                                              |
|------|--------------------------------------------------------------------------------------|
| `0`  | Success                                                                              |
| `2`  | Usage error, unknown action/subcommand, or a missing/invalid argument                |
| `3`  | Encryption/authentication failure (bad or missing key, name mismatch, auth rejected) |
| `4`  | Connection failure or connect timeout                                                |
| `5`  | Operation failed or timed out (e.g. a GATT read/write, modem-pin read, BLE connect)  |
| `6`  | PTY setup failure, or `serial-proxy server` on a non-POSIX platform                  |

Connection errors include a hint when the cause is likely the key or expected name.

---

## Tips and pitfalls

- **Encryption is auto-detected.** Supply the key with `-k`/`--key` (or save it); the CLI picks
  plaintext vs Noise based on the device. A key/no-key mismatch produces a clear error (exit `3`).
- **`switch` is a normal domain here.** Use `esphome-cli <host> switch <id> toggle`; the trailing
  underscore quirk in the library API does not apply to the CLI command name.
- **State is reported asynchronously.** After a control action the CLI waits briefly for the device to
  report the new state, then prints it. The printed state reflects what the device confirmed.
- **Streaming commands run until Ctrl-C** unless you give `bt scan`/`scan` an explicit duration.
- **mDNS needs multicast.** If `scan` finds nothing in a container or VLAN, check that multicast to
  `224.0.0.251:5353` is reachable (use Docker `--network host`).
- **Keys are stored in plaintext** (mode `0600`). Use `--no-save-keys` if you do not want them
  persisted; pass the key via `ESPHOME_CLI_KEY` for one-off use instead.

## Contributing

Contributions to the library are welcome! If you encounter any issues or have suggestions for
improvements,
please feel free to submit a pull request or open an issue on the project's repository.

## License

This project is licensed under the MIT License. See the [LICENSE](../LICENSE) file for details.
