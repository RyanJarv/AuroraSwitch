# Documentation

## Using AuroraSwitch

- [User guide](user_guide.md): build, install, switch, and restore stock firmware.
- [Firmware support](alternative_firmware.md): supported releases and deferred images.
- [How it works](how_it_works.md): lifecycle, memory map, and design choices.

## Development and testing

- [Development guide](development.md): commands and adding firmware.
- [Source map](architecture.md): where each part lives.
- [Verification status](verification.md): current software results and limitations.
- [Reliability checklist](reliability_campaign.md#active-checklist): the active plan.
- [Recovery checklist](recovery_release_gate.md): developer/agent release checks.
- [Load timing](load_timing.md): optional diagnostics; no timeout is implemented.

## Evidence and history

These records retain exact build identities. A pass does not transfer to a new BIN.

- [Seven-image checkpoint](public_release_checkpoint.md) and [machine-readable record](evidence/public_releases_20261006.json).
- [Earlier five-image checkpoint](community_virtual_checkpoint.md).
- [Historical verification](history/verification_20261006.md).
- [Historical campaign](history/reliability_campaign_20261006.md).
- [Historical onboarding](history/alternative_firmware_20261006.md).
- [Initial discovery checks](discovery_checks.md).
- [Normal-boot comparison research](boot_equivalence_plan.md): not a completion requirement.

History preserves old findings and plans; it is not an additional active backlog.
