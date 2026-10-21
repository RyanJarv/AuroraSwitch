# Development

Keep the loader small. Reuse the pinned SDK, libDaisy, FatFs, and SHA-256 code.
Comment non-obvious files, types, and functions briefly: purpose, key invariants,
and their role in the loader. Skip obvious names; explain why, not each statement.

## Build and test

Requires Git, GNU Make, Python 3, C/C++ compilers and GNU Arm Embedded
**10-2020-q4-major** on PATH. Published releases need none of these tools.

```sh
git clone https://github.com/RyanJarv/AuroraSwitch.git && cd AuroraSwitch
make -j2 build
make check
make package
```

See `make help` for virtual build targets. Never install virtual firmware.
Packaging requires clean committed source; it records inputs and checks ELF/BIN
agreement. It does not prove runtime behavior.

On macOS, the pinned Linux toolchain can run through Docker Desktop:

```sh
sh scripts/docker_build.sh make check
sh scripts/docker_build.sh make -j2 build
sh scripts/docker_build.sh make -j2 build-virtual
```

Docker Desktop must be running. The script uses the current checkout and maps
the current user, so generated files remain in this workspace. It builds an
x86_64 Linux image on Apple Silicon; the first build downloads the pinned Arm
toolchain. This matches the exact host-toolchain archive used by release CI;
the firmware target is still Arm Cortex-M. Compilation under emulation may be
slow. `make package` and `make package-virtual` still require a clean committed
checkout. Docker builds do not qualify a physical release.
In Docker Desktop **Settings → Resources → File Sharing**, share this repository
folder before running the script or opening the Docker toolchain in CLion. A
denied bind mount means the container cannot see the project, even when its
compiler checks pass.

For CLion, build the image first with the script above. In **Settings → Build,
Execution, Deployment → Toolchains**, add a **Docker** toolchain using the local
Docker Desktop server and image `aurora-switch-build:10-2020-q4-major`. Set Make
to `/usr/bin/make` and CMake to `/usr/bin/cmake`. Leave the C and C++ compiler
fields on automatic detection so CLion uses `/usr/bin/gcc` and `/usr/bin/g++`
for its own toolchain checks. The project Makefile finds `arm-none-eabi-gcc`
and `arm-none-eabi-g++` on the image's PATH for firmware builds. Do not point
CLion's C++ compiler field at the `bin/` directory or the bare-metal compiler:
CMake's generic link probe cannot link this firmware without its linker setup.
Open the repository's top-level `Makefile` as a Makefile project; this repo has
no `CMakeLists.txt`. In **Makefile** settings, select that toolchain and set the
build target to `build`. Map this project folder to
`/workspace` in the toolchain's container settings if CLion offers a path
mapping. **Build → Build Project** and the build action for CLion's generated
**Native Application** configurations use the Docker toolchain. The configuration
label does not mean compilation runs on macOS. Use the build action: running the
configuration also tries to execute its selected binary, and Arm firmware cannot
run as a native container application.

CLion's **Makefile Target** run configurations do not use Docker toolchains.
For `make check` and `build-virtual` from the IDE, use its terminal or an
External Tool that runs `/bin/sh` with arguments
`scripts/docker_build.sh make check` or
`scripts/docker_build.sh make -j2 build-virtual`, with working directory
`$ProjectFileDir$`.

The selector's discovery, file handling, persistence and handoff logic are shared
with host tests. Builds link either the USB adapter or a synthetic transport;
there are no conditional test branches in the selector/handoff. See
[testing boundaries](testing.md) for fault injection, sanitizers and coverage limits.
The virtual build remains for linked Arm execution and does not test real USB.

From a checkout, `make download-release USB_DIR="/Volumes/AURORA"` automates
checked release/public-firmware downloads and copying; it needs `gh` and host
compilers, but no Arm compiler. `scripts/prepare_payloads.py --output prepared
local-firmware/AR_FDN_v1_2_2.bin` optionally verifies a user-supplied file before
copying. Use a new output folder each time.

## Add firmware

1. Pin the file/version, size, hash, and vectors. Review RAM layout, startup,
   DMA use, inherited clocks/MPU, and persistent writes.
2. Add its path and menu color to [images.hpp](../../firmware/images.hpp).
   Reuse its family's `menu_colors` constant for every version; new families
   need a distinct color within their category. The catalog test checks RGB
   proportions to reject close colors, independent of brightness; check the
   actual LEDs on hardware too. Keep identities and review staging changes.
3. Extend catalog/authentication tests. Run `make check` and validate local
   payloads with `prepare_payloads.py` or `make verify-images`.
4. Commit and package. Test linked launch, controls/audio, copy, and DMA cleanup.
   Handoff changes also need complete switching-cycle tests.
5. Record exact builds and limits, then test physical launch and stock recovery.

Do not commit payloads. Host tests alone do not prove compatibility.
Do not create a new emulator for a difficult image; defer it.

## Working notes

- [Firmware onboarding record](firmware_onboarding.md): older versions, Tempest,
  and the FataMorgana RAM experiment. Intake is not supported-image evidence.
- [Architecture](architecture.md): source map, memory, and handoff.
- [QSPI branch review](branch_review_20261007.md): tooling fixes and remaining test boundaries.
- [Builds and releases](releases.md): tag workflow and build-only CI.
- [Verification status](verification.md) and [active checklist](reliability_campaign.md#active-checklist).
- [Recovery checks](recovery_release_gate.md) and [optional timing diagnostics](load_timing.md).
- [QSPI feasibility](qspi_feasibility.md): updater boundaries and Dirt Verb implementation.
- [Persistent boot](persistent_boot.md): remembered choices and the private Dirt resident installer.
- [Additional-image checkpoint](qspi_additional_images.md): current
  Dirt/HP-filter scope and historical QSPI Fata evidence.
- [Seven-image checkpoint](checkpoints/public_releases_20261006.md) and [machine-readable evidence](evidence/public_releases_20261006.json) (history).

Historical records stay in `history/`. Do not transfer their results to changed
binaries. Documentation-only edits need link checks, not new emulator campaigns.
