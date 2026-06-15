Implemented Phase 2.2.

Removed:
- 25 dead plain decoder functions from [src/uci_plain_decoders.c](/media/chpo/HDD-papa/gemini_test/uci_interactive_shell/src/uci_plain_decoders.c)
- Their public declarations from [include/uci.h](/media/chpo/HDD-papa/gemini_test/uci_interactive_shell/include/uci.h)
- 16 dead `ui_decode_*` functions from [src/uci_ui_packet_decoder.c](/media/chpo/HDD-papa/gemini_test/uci_interactive_shell/src/uci_ui_packet_decoder.c)
- Their declarations from [include/uci_ui_packet_decoder.h](/media/chpo/HDD-papa/gemini_test/uci_interactive_shell/include/uci_ui_packet_decoder.h)
- Obsolete plain-decoder-only tests from [tests/test_uci_functions.c](/media/chpo/HDD-papa/gemini_test/uci_interactive_shell/tests/test_uci_functions.c)

Kept `decode_range_vendor_data` and the range measurement print helpers because `src/uci.c` still calls them.

Verification passed:
- `make`
- `make unit-test`
- `make command-handler-test`
- `make command-framework-validation-test`

Final dead-symbol scan found no remaining references/declarations for the removed decoder names. Note: the worktree already had unrelated dirty files (`Makefile`, `docs/ARCHITECTURE.md`, deleted packet-structures files, `.adversarial-loop/`); I left those untouched.