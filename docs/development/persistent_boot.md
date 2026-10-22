# USB saved choice

The selector reads `0:/aurora/choice.dat`: exactly 76 bytes containing `AURUSB01`,
64 lowercase SHA-256 characters identifying a current catalog original, then a
little-endian CRC-32/ISO-HDLC over the preceding 72 bytes. Unknown hashes, wrong
versions, lengths or CRCs provide no automatic launch authority.

Read the saved choice after the first successful scan. Authenticate its actual
file before launching. The startup button combination or explicit navigation
cancels automatic boot; reinsertion after an attempted boot does not retry it.

For a manual authenticated choice, reread the record first. If it already
identifies that image, do not open it for writing. Otherwise replace it once,
check full write, sync, close and validated readback. Save failure warns and
permits that session's checked launch. No background retry occurs. A torn write
can discard the previous choice; this acceptable loss returns to the menu.

All persistence is on USB. The selector does not initialize QSPI or any on-module
persistent settings. The stock updater necessarily writes the selector image
when installing; vendor activity after handoff is measured separately.
