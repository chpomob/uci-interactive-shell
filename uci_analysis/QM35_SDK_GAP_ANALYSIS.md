# QM35 SDK vs. `uci-shell` Gap Analysis

This note highlights concrete gaps between our current shell implementation and the UCI patterns exercised inside the Qorvo QM35 SDK. Each item references shell sources to show today’s behavior and explains why the SDK approach would improve fidelity or maintainability.

## 1. Message Dispatch Architecture
- **SDK pattern**: per-group handler tables with automatic `known_gid` bookkeeping and a generic fallback let the core reply with `UNKNOWN_GID/OID` without hand-written `switch` trees.
- **Shell today**: `send_uci_command` implements a monolithic `if/else` to recognize every command and craft synthetic responses (`src/uci.c:597`). When a message is not covered we silently fall through, so no status packet is returned and the CLI only prints a decoded payload (`src/uci.c:2386`).
- **Impact**: Adding new opcodes requires editing this giant function, and negative testing is impossible because the simulator never emits protocol errors like the SDK’s `uci_send_status` paths.
- **Recommendation**: Introduce lightweight handler tables (e.g. `struct { uint16_t id; handler_fn fn; }`) keyed by `MT/GID/OID`, reuse them for both simulation and decode, and generate default `UNKNOWN_GID/OID` responses when lookups fail.

## 2. Segmentation & Reassembly Flow
- **SDK pattern**: `uci_packet_recv()` keeps partially assembled chains, validates continuity, and only hands complete messages to the dispatcher.
- **Shell today**: `uci_hw_interface` accumulates fragments in a single global buffer (`src/uci_hw_interface.c:15-173`) but returns raw packets straight to callers; nothing queues completed chains or shields `parse_uci_packet` from partial traffic.
- **Impact**: Any caller must remember to poll `uci_fragment_process` until it returns a full packet. We also cap reassembled payloads at 1024 bytes and drop overflow without issuing status notifications.
- **Recommendation**: Adopt the SDK approach—track an RX head/tail inside the interface, expose a `uci_packet_recv()` that finalizes segmentation, and surface errors through synthetic status packets so higher layers do not have to reimplement those guards.

## 3. Message Construction Utilities
- **SDK pattern**: `uci_message_builder` reserves header space, handles multi-block payloads, and supports nested TLVs while reusing the same allocator everywhere.
- **Shell today**: Every command constructs payload arrays manually (`src/uci_packet_utils.c:5-99`) and duplicates LE helpers that also live in `uci.c:1-46`. There is no support for segmentation during transmission—we always send a single 255-byte payload.
- **Impact**: Helper duplication risks divergence, and we cannot exercise large responses or data packets, which the SDK covers by default.
- **Recommendation**: Centralize LE helpers and add a simple builder that pre-pends headers and chunks payloads at 255 bytes, plus a send path that emits chained packets when needed.

## 4. TLV Parsing & Config Storage
- **SDK pattern**: shared helpers (`tvs_to_bytes`, `tlvs_from_bytes`) convert TLVs across both C and Python stacks, so command handlers stay declarative.
- **Shell today**: Each decoder walks TLVs by hand (e.g. `decode_core_get_caps_info_rsp` at `src/uci.c:2467-2534`), and the config manager hardcodes every parameter in arrays (`src/uci_config_manager.c:1-160`).
- **Impact**: We risk inconsistent parsing logic and spend effort keeping duplicate tables in sync. Adding a new TLV means editing multiple switch statements.
- **Recommendation**: Port the SDK’s TLV helpers (possibly the Python versions first for the CLI) and replace ad-hoc loops with declarative definitions.

## 5. Transport Abstraction & Backpressure
- **SDK pattern**: transports expose `attach/detach`, queue packets via `uci_packet_send_get_ready`, and rely on epoll to drain TX while reporting failures back to the core.
- **Shell today**: The simulator prints packets and immediately calls `parse_uci_packet` for the synthetic response (`src/uci.c:540-775`), while hardware mode just dumps bytes without a send queue. No component owns retransmission or reports `packet_send_done` status codes.
- **Impact**: We cannot emulate transport-level delays, inject IO failures, or reuse the interface for different backends. Features like retry-on-fail or send credit accounting are effectively untestable.
- **Recommendation**: Wrap outbound packets in a queue structure, trigger draining via a transport callback (reusing the SDK’s `packet_send_ready` idea), and surface send completion so higher layers can schedule retries.

## 6. Device-State & Notification Model
- **SDK pattern**: the core guards command handling with `uci_get_device_state` and pushes boot/device-status notifications automatically.
- **Shell today**: Device state is stored in `struct uci_session` but only updated in a few command branches; nothing trusts it when processing inbound frames, and we enqueue notifications manually in each branch (`src/uci.c:613-819`).
- **Impact**: Our simulator happily accepts commands even if we previously sent an error state and never emits the matching status transitions unless the branch author remembered to enqueue them.
- **Recommendation**: Track device state centrally and gate command handlers the same way the SDK does (reset-only while in `ERROR`, etc.), generating notifications from a single place instead of duplicating logic across branches.

## 7. Data Message Handling
- **SDK pattern**: transports accept `DATA_MESSAGE_SND/RCV` packets, chunk large application payloads automatically, and surface `SESSION_DATA_TRANSFER_STATUS_NTF` together with credit updates so higher layers can exercise in-band data delivery.
- **Shell before**: the CLI had no command to emit data packets, the simulator ignored message type `0x0`, and sessions never recorded transfer metadata, making it impossible to mirror the SDK's in-band data flows.
- **Impact**: We could not validate data-plane behaviour, exercise segmentation rules, or observe credit/status notifications the way the QM35 stack does.
- **Update**: `uci_send_data_message` now builds DATA_MESSAGE_SND payloads, segments them at 255 bytes, updates session state, and synthesises credit/status notifications in the simulator. The CLI command `session_send_data` mirrors the SDK tooling, and the packet analyzer understands DPF-prefixed frames so comparison with the QM35 SDK is straightforward.

These updates would move the CLI much closer to the SDK’s behaviour and make future comparisons or protocol validation far easier.
