# Category menu

Current source uses Shift for category, Reverse for selection, and Freeze for
reload/authenticate/launch. Firmware colors and payload identities are unchanged.
The board has no Shift LED: one SDK-mapped arc displays category color and
within-category position. Empty categories are skipped.

Host menu tests cover wrapping, missing entries/groups, rescan and disconnect.
Existing staging, handoff and QSPI fault tests retain the launch-time safeguards.
Historical virtual switching records belong to their old images and three-button
protocol; they do not qualify this changed selector. No handoff cleanup or
payload support changed.

The fresh companion for source `cf8df78` passes the live VM menu/LED screen,
both official Freeze-only launches, exact payload/DMA cleanup checks, and 85
balanced audio callbacks (8,192 stereo frames) per official image. Changed
launch-read bytes reject; omitting Freeze never launches. These runs use
synthetic media and direct SRAM startup, not real USB or the updater.
The new exact build still needs a physical menu and
launch screen, with ST-Link disconnected, plus ordinary stock recovery testing.
