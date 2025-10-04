# UCI Command Support Overview

This CLI now simulates a subset of the Android UCI command surface to unblock
host tooling. The following control flows are explicitly handled:

- **CORE**: device info, capability query, set/get config, device reset,
  suspend, and UWBS timestamp query.
- **SESSION CONFIG**: session init/deinit, set/get app config, get count/state,
  update controller multicast list, DT tag updates, query max data size, and
  hybrid/test configuration stubs.
- **SESSION CONTROL**: start/stop sessions and ranging-count queries.
- **VENDOR ANDROID**: country code, power stats, and radar config commands.
- **TEST**: RF test configuration, periodic TX, PER RX, generic RX, and stop
  commands.

Key limitations to keep in mind:

- Data packets are not transported; data/control notifications are mocked only
  when explicitly emitted inside `send_uci_command`.
- TLV values are stored as single-byte placeholders, so complex multi-byte
  parameters will be truncated.
- Session handles are synthesized locally and do not survive resets.
- Unsupported commands fall back to `UCI_STATUS_OK` without side effects.

When adding new features, ensure the corresponding command path returns a
well-formed status payload and, where practical, validates incoming buffers to
match the patterns documented in the Android UCI manager.
