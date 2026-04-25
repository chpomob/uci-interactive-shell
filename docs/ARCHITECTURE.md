# Architecture

This document describes the architecture of the **UCI Interactive Shell** (the CLI host).
See also `docs/COMMAND_REFERENCE.md` for the full command list and `PROJECT_STATUS.md`
for the current project direction.

---

## 1. High-level Design

```
┌─────────────────────────────────────────────────────────┐
│  readline / history / terminal                           │
├─────────────────────────────────────────────────────────┤
│  uci_shell_run_interactive()  (src/uci_shell_runtime.c)  │
│  - readline loop, command dispatch                       │
├─────────────────────────────────────────────────────────┤
│  Command Framework                                       │
│  src/uci_command_framework.c                             │
│  src/uci_cmd_framework_*.c                               │
│  • Declarative command tables (name → handler)           │
│  • Aliases built automatically                           │
│  • Param parsing & validation typed by name              │
│  • Completion auto-generated from table                  │
├──────────────────────────┬──────────────────────────────┤
│  Handler Layer           │  Protocol Layer              │
│  src/uci_cmd_*.c         │  src/uci_packet_*.c          │
│  src/uci_cmd_*.h         │  src/uci_qorvo_utils.c       │
│  • session / config      │  • packet building           │
│  • hw / simulation       │  • packet decoding           │
├──────────────────────────┴──────────────────────────────┤
│  Transport Layer                                       │
│  src/uci_hw_*.c                  │  src/uci_tcp_transport.c     │
│  src/uci_simulator.c             │                            │
│  • chardev send / recv           │  • TCP send / recv          │
├─────────────────────────────────────────────────────────┤
│  Storage                                                  │
│  src/uci_session_store.c  │  src/uci_config_manager.c     │
│  • live session structs   │  • config table / defaults    │
└─────────────────────────────────────────────────────────┘
```

## 2. Command Framework

The command system is **declarative**. Each command is defined once as a
`UciCmdEntry` (or `UciTypedCmdEntry`) struct containing:

| Field         | Purpose                                |
|---------------|----------------------------------------|
| name          | Canonical command name                 |
| help          | One-line help shown by `help` / completion |
| handler       | `int cmd(ctx, argv, argc)` callback    |
| args[]        | Ordered list of typed arguments (name, type, description) |
| completions[] | Readline _rl_complet function list     |
| aliases[]     | Extra names (e.g. `get_caps_info` ←→ `caps_info`) |

**Key properties:**

- **`help` always matches the framework tables** — no doc/code drift possible.
- **`tab` completion regenerates from the same tables** — no separate completion list.
- **Parameter validation** happens before the handler fires, so `argv` is pre-checked
  and the handler only deals with validated integers / strings / hex arrays.
- **Session context auto-injection**: the framework stores the active session ID
  (`g_current_session`) and passes it to handlers that request it in their `args[]`.

### Command metadata sources

| file                                      | contents                           |
|-------------------------------------------|------------------------------------|
| `src/uci_command_framework.c`            | core dispatch, alias resolution    |
| `src/uci_cmd_framework_bridge.c`         | framework → legacy handler glue    |
| `src/uci_cmd_framework_device.c`         | device-management command entries  |
| `src/uci_cmd_framework_session.c`        | session-config/control entries     |
| `src/uci_cmd_framework_simulation.c`     | simulation / demo entries          |
| `src/uci_cmd*.c` (typed versions)        | validated handlers for `get_config` `set_config` `set_power` `validate_arguments` |
| `src/uci_cmd_session_config_ext.c`       | extended session configuration     |

### Adding a new command

```c
/* 1. Add to the command table */
static const UciTypedCmdEntry kMyCmdArgs[] = {
    { "id",  UCI_ARG_INT,    "Session ID" },
    { "value", UCI_ARG_HEX_ARRAY, "Binary payload" },
};

static const UciCmdTableEntry g_cmd_table[] = {
    { "my_new_command", "Do something cool", CMD_TypedHandler, kMyCmdArgs, {NULL}, {} },
    ...
};

/* 2. Write the handler */
int uci_cmd_my_new_command(UciShellCtx *ctx, const UciParsedArg *args, int argc) {
    int id      = args[0].v.int_;
    uint8_t *payload = args[1].v.hex.buf;
    size_t     len = args[1].v.hex.len;
    ...
}
```

No documentation, no completion, or validation code to write. The framework
propagates `--help` text and tab-completion automatically.

## 3. Protocol Layer

### 3.1 Constants (`uci_pdl.h`)

Source of truth for all UCI wire protocol values:

- Standard UCI group / opcode codes (from Android UWB spec).
- Qorvo vendor-group mappings (from Cherry C headers), notably `GID 0x0E = QORVO_MAC`.
- Session state, device state, and session reason enumeration values.

All code **must** reference `UciPdl*` / `UciOpcode*` constants from this header.

### 3.2 Packet Building (`include/uci_packet_structures.h`)

```c
typedef struct { UciPdlHdr hdr; UciPdlMsg msg; UciPdlData data; } UciPdlPacket;
typedef struct { UciPdlSessionHdr hdr; SessionPayload payload; } UciPdlSessionMsg;
```

- `UciPdlHdr` — the UCI wire header (msg type, payload size, group ID, opcode).
- `UciPdlData` — variable-length payload; struct size = fixed header portion.

Packet helpers in `src/uci_packet_structures.c` and `src/uci_packet_utils.c` fill
the fixed header and compute wire length. Outbound `DATA` packets use a 16-bit
little-endian length (bytes 2-3); control/response/notification packets use an
8-bit length (byte 2).

### 3.3 Packet Decoding

Three parallel decode surfaces — all share identical semantics via `uci_packet_utils.c`
lookup tables:

| surface                        | uses shared enums / labels?   |
|--------------------------------|-------------------------------|
| `src/uci.c` (plain CLI output) | yes                           |
| `src/uci_packet_analyzer.c` (enhanced packet analyze) | yes   |
| `src/uci_ui_packet_decoder.c` (UI labels) | yes                |

Changes to a protocol enum (e.g., `SESSION_STATUS_RUNNING`) propagate to all three
surfaces in one edit.

## 4. Transport Layer

| transport           | source                     | key types                       |
|---------------------|----------------------------|---------------------------------|
| chardev (hardware)  | `src/uci_hw*.c`            | `UciHwCtx`                      |
| TCP simulator       | `src/uci_tcp_transport.c`  | `UciTcpCtx`                     |
| simulation          | `src/uci_simulator.c`      | `UciSimCtx`                     |

All transports implement the same interface:

```c
int uci_send(UciTransportCtx *ctx, const void *buf, size_t len);
int uci_recv(UciTransportCtx *ctx, void *buf, size_t max, UciPacket **out);
void uci_transport_destroy(UciTransportCtx *ctx);
```

The active transport is stored in `g_uwb_chardev` (which is a void* union).
`mode_hw` / `mode_tcp` / `mode_sim` swap the active transport and its send/recv
wrappers.

## 5. Configuration Manager

`src/uci_config_manager.c` — centralized `set_config` / `get_config` backend:

- Reads device config defaults from a table in `src/uci_config_metadata.c`.
- Validates ranges, writes to a persistent key/value store (SQLite, in-memory fallback).
- Exposed via `get_config <name>` and `set_config <name> <value>`.

## 6. Session Store

`src/uci_session_store.c` — singleton session table:

- `SessionEntry` structs keyed by numeric session ID.
- Auto-creates new session on `session_init`.
- `get_session_state`, `get_session_count`, and `session_start` pull from this store.

## 7. Key Design Decisions

1. **Single source of truth**: all protocol values flow through `uci_pdl.h`. No embedded
   numeric literals in handlers or analyzers.
2. **Declarative commands**: adding a command is a 3-line table edit + one handler.
3. **Transport-agnostic core**: session management and config live entirely outside
   transport code. Any new transport (e.g., pseudo-serial) plugs in cleanly.
4. **Shared decode helpers**: CLI output, packet analyzer, and UI decoder all share
   `uci_packet_utils.c` lookup tables. One enum change updates all surfaces.
5. **Typed argument parsing**: handlers receive pre-validated arguments (int, hex array,
   string, bool), eliminating `atoi` and `strtol` error paths in handlers.

## 8. File Map

| category            | files (src/)                                    |
|---------------------|------------------------------------------------|
| Core runtime        | `main.c`, `uci.c`, `uci_shell_runtime.c       `|
| CLI framework       | `uci_command_framework.c`, `uci_command_utils.c` |
| Command tables      | `uci_cmd_framework_bridge.c`, `uci_cmd_framework_device.c`, `uci_cmd_framework_session.c`, `uci_cmd_framework_simulation.c` |
| Command handlers    | `uci_cmd_core.c`, `uci_cmd_core_typed.c`, `uci_cmd_session.c`, `uci_cmd_session_config.c`, `uci_cmd_session_config_ext.c`, `uci_cmd_analysis.c`, `uci_cmd_hardware.c`, `uci_cmd_hardware_typed.c`, `uci_cmd_handlers_session.c`, `uci_cmd_handlers_simulation.c` |
| Protocol / packets  | `uci_packet_structures.c`, `uci_packet_utils.c`, `uci_decode_utils.c`, `uci_plain_decoders.c`, `uci_qorvo_utils.c` |
| Transport           | `uci_hw_interface.c`, `uci_hw.c`, `uci_hw_chardev.c`, `uci_tcp_transport.c`, `uci_simulator.c` |
| Storage             | `uci_session_store.c`, `uci_config_manager.c`, `uci_config_metadata.c` |
| UI                  | `uci_ui.c`, `uci_ui_main_patch.c`, `uci_ui_packet_decoder.c`, `uci_ui_range_decoder.c` |
