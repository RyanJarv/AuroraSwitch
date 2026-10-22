# Development builds and releases

`make package` seals a fresh build of clean committed source and pinned SDK,
libDaisy and mbedTLS. GNU Arm 10-2020-q4-major is required. ELF/BIN agreement,
tool identities, source identity and exact artifact inventory are checked.

`bash scripts/docker_build.sh` runs host checks using the Docker daemon's native
AMD64 or ARM64 architecture and the matching GNU Arm compiler archive. Pass a
command to run it in the same container, for example
`bash scripts/docker_build.sh make build`. ARM here means a 64-bit ARM Linux
host; the firmware target remains the Aurora's Cortex-M7 on either host.
The wrapper handles rootless Docker UID mapping so bind-mounted build outputs
remain writable without changing host directory permissions.

`make preparer` also exports a self-contained `*-preparer.html` in `dist/`.
It embeds only our selector, startup wrapper and catalog metadata. Users supply
original firmware locally in a browser and receive one private installation BIN.
No vendor firmware, generated installer or user file belongs in release assets.

The release workflow uploads the preparer, provenance and SHA256SUMS. The raw
selector BIN is a development input, not a standalone flash-bundle installer.
Checks bind embedded selector bytes to the sealed manifest and release source.
New releases remain drafts while physical qualification is open. A hosted build
is not authorization to publish or deploy hardware.

The stock updater and its USB recovery mechanism remain unchanged. Virtual
startup and modeled flash readback do not qualify real USB programming, power
interruption, audio or ordinary stock restoration. See the
[release gate](recovery_release_gate.md).
