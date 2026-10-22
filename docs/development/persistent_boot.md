# Installed firmware and saved choices

The [installed layout and launch sequence](../how_it_works.md) describe the
current fixed-slot design. Both preparers consume offsets exported from the
compiled catalog. Every installer covers the full slot area, filling omitted
images with erased bytes so a previous optional image cannot survive a subset
replacement. Expected vectors determine menu presence; full copied-image
authentication determines whether launch is allowed.

## Selection journal

`ChoiceJournal` owns two 4-KiB sectors outside the installer. Each sector has
sixteen page-aligned slots for 80-byte records: magic, generation, exact image
hash and CRC. Hashes keep choices independent of catalog order. CRC rejects
incomplete records; generation determines which complete record is newest.

A changed explicit choice appends to the sector opposite the newest record.
Automatic boots and repeated choices do not initialize QSPI or program flash.
Recycling erases the older sector while preserving the newer committed choice.
After an interrupted erase, append after the last occupied page rather than
reusing an earlier hole. Torn records are skipped. If neither sector has a
valid record, an explicit save can initialize one; ambiguous ordering and
readback failures prevent saving.

A failed save requests a brief red warning and still attempts the checked
handoff. The next boot trusts the valid record actually present, not the
reported success of the previous session. Dirt handoff requires mapped flash;
a mapping failure cannot be bypassed by the session-launch policy.

## Updater boundary

The bundle ends before the journal with 68 KiB reserved for the stock updater's
possible extra EOF erase and inclusive final sector. See the
[stock updater notes](stock_updater.md). Choice records are outside its file-byte
comparison, so keeping an unchanged installer on a drive does not make changed
preferences look like changed application firmware.

The pinned SDK issues status-register writes while changing QSPI modes. These
are measured separately from array programming/erase. Vendor firmware can make
its own settings writes after handoff; the selector is not a sandbox.

## Evidence

The [fixed-slot record](evidence/fixed_slot_validation_20261021.json) includes
32 production scenarios, all 14 handoffs, failed-save/reboot checks, host and
browser checks, and the merged build's byte identity. Earlier records in
`evidence/` remain bound to their named source/artifact identities. Virtual tests
do not qualify physical USB installation, power interruption, audio or stock
restoration; those remain the [release gate](recovery_release_gate.md).
