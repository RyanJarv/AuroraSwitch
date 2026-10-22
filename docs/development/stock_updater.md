# Stock updater assumptions

The reviewed loader is pinned Daisy v5.4, SHA-256
`b442456ab1c7f9689b73e57fb09ca55331d02760b54be28c10a207afed95f06e`.
These notes preserve the relevant conclusions of the earlier switching
investigation; they do not establish physical installation or recovery.

- The updater searches the USB root for a BIN. Keep only one root BIN; filenames
  are not a panel-selection mechanism.
- It compares file bytes against flash beginning at `0x90040000` and skips an
  unchanged file. The comparison covers file length, not the unused flash tail.
- Its application destination is fixed. Storing a vendor image at another
  address does not relocate addresses embedded in that binary.
- A RAM-linked application is copied to `0x24000000`; a QSPI-linked image runs
  at the application flash address. The resident wrapper gives the selector an
  entry while retaining Dirt's required location.
- The old updater erases before its next file read. A file ending on a 64-KiB
  boundary can cause an extra erase on the final EOF read. Its linked eraser
  also includes the final 4-KiB sector. The current SDK's eraser has a different,
  exclusive-end contract. `SELECTOR_BUNDLE_LIMIT` reserves room for both effects.
- The old update loop does not check all read/erase/write results and offers no
  atomic installation or rollback. A success message is not readback evidence.

Flash below `0x90040000` is reserved: calibration includes offset `0x1000` and
vendor settings include `0x2000`. The selector never programs this region or
internal bootloader flash. Selection writes are bounded to the two journal
sectors, independently of the image-slot area.

Static linked landmarks for this exact loader: byte comparison
`0x080061d6..0x0800625c`, matching-file skip `0x080062da..0x080062e2`,
erase/write calls `0x08006330`/`0x0800635a`, and application selection
`0x080059c8..0x080059fa`. The reference source helps interpret the captured
binary; source-to-binary reproducibility was not established. The original
analysis remains available with `git show 0359fd7:docs/development/qspi_feasibility.md`.

Normal USB restoration must be qualified on the exact candidate and hardware
before publishing a firmware release. See the [release gate](recovery_release_gate.md).
