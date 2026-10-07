# Historical normal-boot comparison research

This is not an active plan. Use the
[reliability checklist](../reliability_campaign.md#active-checklist).

Whole-machine equality is not a useful requirement: normal boot stores the
payload in QSPI, while selector boot stores the selector there. RAM leftovers,
timers, and USB history can also differ. Compare behavior and the startup
conditions a target actually depends on, not every byte or controller register.

## Preserved findings

An earlier virtual selector, BIN
`13ee50f9079535ef8d8d06a90f6119a7e4434874ff9b56dfb9f0771a5b901e60`,
showed two differences despite matching bounded startup audio:

- FDN's prewrite-memory block changed three times versus two in its baseline.
- Aurora's timer overflow/counter values differed.

These findings do not establish whole-state or timing equivalence, and do not
describe the current selector. Investigate additional state only for a
demonstrated failure or concrete hazard. Do not hide differences through host
memory clearing or unexplained normalization.

Completed switching-cycle evidence remains image-specific. Synthetic media
cannot prove USB/FatFs behavior or physical recovery. A portable standalone
virtual-test recipe remains a follow-up; do not build another emulator here.

The longer proposed comparison campaign is retained in Git history, not as an
additional completion checklist.
