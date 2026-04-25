# UCI Shell Command Reference

> **Architecture note:** All commands are driven by a declarative command framework.
> Command metadata (names, aliases, help text, parameter types) lives in
> `src/uci_cmd_framework_*.c` with handlers in `src/uci_cmd*.c`. Updating a
> single command entry propagates to help output, readline completion, validation,
> and dispatch automatically.

## General / UI

- `help` — Show help message
- `analyze_packet [--verbose] [--tlv] [--compare] [--help] <bytes...>` — Run enhanced packet analyzer
- `quit` — Exit the shell

## Device Management

- `get_device_info` — Query device information (alias: `device_info`)
- `device_reset` — Reset the connected device
- `set_power <state>` — Set device power state (alias: `device_on`, `device_off`)
- `get_caps_info` — Query capability information
- `get_config <config>` — Read a device configuration parameter
- `set_config <config> <value>` — Write a device configuration parameter
- `device_suspend` — Suspend the device
- `show_device_configs [--id <hex | decimal>] [--filter <substring>] [ --full | --summary ]` — List device configuration entries
  - `--full` shows all device config entries (not just default values)
  - `--id <hex | decimal>` filters to a single parameter by ID
  - `--filter <substring>` filters by name substring

## Session Configuration & Control

- `session_init <id> <type>` — Initialize a session
- `session_deinit <id>` — Deinitialize a session
- `session_start <id>` — Start a session
- `session_stop <id>` — Stop a session
- `get_session_state <id>` — Get session state
- `get_session_count` — Get session count
- `set_app_config <id> <param> <value>` — Set application config
- `get_app_config <id> [param ...]` — Get application config
- `session_update_multicast_list <id> <action> <short> <subsession>` — Update multicast list
- `session_update_dt_tag_rounds <id> [rounds ...]` — Update DT-Tag rounds
- `session_update_dt_anchor_rounds <id> [rounds ...]` — Update DT-Anchor rounds
- `session_data_transfer_phase_config <id> <repetition> <control> <size> [payload ...]` — Configure data transfer
- `session_set_hybrid_controller_config <id> [hex-config]` — Set hybrid controller config
- `session_set_hybrid_controlee_config <id> [hex-config]` — Set hybrid controlee config
- `session_query_data_size_in_ranging <id>` — Query data size
- `session_send_data <id> <dest> <seq> <payload>` — Send data
- `session_query_ranging_count <id>` — Query ranging count
- `session_logical_link_create <id> <link_id> <mode> <credit>` — Create logical link
- `session_logical_link_close <id> <link_id>` — Close logical link
- `session_logical_link_get_param <id> <link_id> <param>` — Get link parameter

## Simulation

- `simulate_ranging` — Simulate ranging notification
- `simulate_multi_target_ranging` — Simulate multi-target ranging
- `simulate_notification` — Simulate generic notification
- `simulate_session_status` — Simulate session status notification
- `simulate_data_credit` — Simulate data credit notification
- `demo_session_flow` — Demo full session flow

## Hardware Mode

- `mode_hw <device_path>` — Switch to hardware mode (alias: `hw_init <path>`, `hw_connect <path>`)
- `mode_tcp <host> <port>` — Switch to TCP simulator mode
- `mode_sim` — Switch to simulation mode
- `mode_info` — Display current mode
- `hw_send <mt> <pbf> <gid> <oid> [payload ...]` — Send raw packet in hardware mode

## Protocol Constants

The following Group ID (GID) constants are authoritative and live in `include/uci_pdl.h`:

| Constant          | Value | Description                         |
|-------------------|-------|-------------------------------------|
| CORE              | 0x00  | Core device management              |
| SESSION_CONFIG    | 0x01  | Session configuration               |
| SESSION_CONTROL   | 0x02  | Session control                     |
| DATA_CONTROL      | 0x03  | Data transfer control               |
| RANGING_DATA      | 0x0B  | Ranging data notifications          |
| VENDOR_ANDROID    | 0x0C  | Android vendor extensions           |
| TEST              | 0x0D  | Test commands                       |

## App Config Reference (Key Parameters)

App configurations are session-specific. Key application configuration parameters:

| ID    | Parameter Name                              |
|-------|---------------------------------------------|
| 0x00  | device_type                                 |
| 0x14  | preamble_code_index                         |
| 0x1B  | slots_per_rr                                |
| 0x1E  | responder_slot_index                        |
| 0x25  | session_priority                            |
| 0x28  | static_sts_iv                               |
| 0x29  | number_of_sts_segments                      |
| 0x2A  | max_rr_retry                                |
| 0x3A  | min_frames_per_rr                           |
| 0x3B  | mtu_size                                    |
| 0x45  | session_key                                 |
| 0x46  | subsession_key                              |
| 0x47  | session_data_transfer_status_ntf_config     |
| 0x48  | session_time_base                           |

> **Tip:** Use `show_app_configs --full` and `show_device_configs --full` for complete metadata (defaults, valid ranges, units).
