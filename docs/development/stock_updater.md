# Stock updater assumptions

The reviewed loader is pinned Daisy v5.4, SHA-256
`b442456ab1c7f9689b73e57fb09ca55331d02760b54be28c10a207afed95f06e`.
These notes preserve the relevant conclusions of the earlier switching
investigation; they do not establish physical installation or recovery.

- The updater searches the USB root for a BIN. Keep only one root BIN; filenames
  are not a panel-selection mechanism. It matches `.bin`/`.BIN` substrings;
  inactive backups must contain neither. Dirt selection requires an empty root
  BIN set, then moves the authenticated original to `DIRTVERB.BIN` and resets.
- It compares file bytes against flash beginning at `0x90040000` and skips an
  unchanged file. The comparison covers file length, not the unused flash tail.
- Its application destination is fixed. Storing a vendor image at another
  address does not relocate addresses embedded in that binary.
- A RAM-linked application is copied to `0x24000000`; a QSPI-linked image runs
  at the application flash address. AuroraSwitch directly launches only RAM-linked images; Dirt selection prepares
  its unchanged BIN for stock installation and resets.
- The old updater erases before its next file read. A file ending on a 64-KiB
  boundary can cause an extra erase on the final EOF read. Its linked eraser
  also includes the final 4-KiB sector. The current SDK's eraser has a different,
  exclusive-end contract. Selector packaging stays within the ordinary
  application size; there is no separate journal or reserved image tail.
- The old update loop does not check all read/erase/write results and offers no
  atomic installation or rollback. A success message is not readback evidence.

Flash below `0x90040000` is reserved: calibration includes offset `0x1000` and
vendor settings include `0x2000`. The selector never programs this region or
internal bootloader flash. The selector itself performs no on-module
programming, erasing, settings initialization or protection changes.
