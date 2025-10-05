# Agent Guide: `uci_analysis/`

## Purpose
Reference material pulled from Android's UWB stack plus local notes that document protocol expectations for the UCI shell.

## Contents
- `SUMMARY.md`: High-level comparison between Android implementation and this project.
- `UCI_PROTOCOL_ANALYSIS.md`: Detailed walkthrough of message types, TLVs, and behaviors from the Android spec.
- `uci_packet_generator.py`: Python helper to craft UCI packets for testing/experimentation.
- `uwb/`: Snapshot of upstream Android definitions (PDL, metadata).

## Usage Guidelines
- Treat files as documentation/support tools—do not depend on them at runtime.
- When adding new protocol support, update `SUMMARY.md` or `UCI_PROTOCOL_ANALYSIS.md` to reflect coverage and outstanding gaps.
- Use `uci_packet_generator.py` to produce test inputs for new decoders; include sample invocations in test comments when helpful.
- Preserve upstream licensing in `uwb/`; avoid modifying files mirrored from Android unless clearly marked as local extensions.

## Suggested Workflow for Agents
1. Skim `SUMMARY.md` to understand current alignment with Android UCI spec.
2. Consult `UCI_PROTOCOL_ANALYSIS.md` before implementing new commands/TLVs to ensure compliance.
3. Leverage the packet generator during test development; keep scripts in sync with any local enum changes.
4. Document protocol assumptions or deviations here so other contributors can cross-reference quickly.
