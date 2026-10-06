# Design

The application is linked with the pinned libDaisy `BOOT_SRAM` layout at
`0x24000000`. It inherits the recovery bootloader's clocks, SDRAM/QSPI mappings
and MPU configuration. It deliberately avoids a second SDRAM/QSPI setup.

USB-host/FatFs and Mbed TLS SHA-256 are reused upstream components. The selector
reads once into bounded internal SRAM, checks exact size, hash and vectors,
then validates the same bytes again immediately before handoff. It never
validates one file and launches a separately reopened copy.

Supported-image discovery is a bounded walk of the compiled compatibility
catalog, not general directory enumeration or arbitrary-BIN support. It reuses
that same read-only staging and exact authentication path for each catalog
filename at initial media readiness and after disconnect/reconnect. A tiny
transport-independent menu helper remembers only authenticated availability
and cycles over it. A disconnect during any probe clears the whole menu.
Discovery overwrites staging but never marks it launchable: Freeze must reload
and reauthenticate the selected file, then the handoff verifies those staged
bytes again. Missing, corrupt, mismatched and unknown images cannot gain an
entry or execution permission. Adding a new image still needs a reviewed
catalog/memory contract, tests and a selector rebuild.

FatFs objects are aligned inside the DMA-accessible arena. Staging starts at
`0x30008000`, outside that arena, and ends at `0x30034680` for the supported
image set (181888-byte capacity). File writes, unlink, persistent storage and
QSPI erase/write entry points are not linked into the audited build.

The handoff sequence unmounts/stops media, masks interrupts, resets the scoped
DMA/USB/I2C/SAI bus masters, deinitializes the runtime, clears SysTick and NVIC
state, preserves MPU/clock/FMC/QSPI configuration, and installs a stackless
SRAM4 trampoline at `0x38000000` (reserved through `0x38000400`).

The trampoline copies the exact image to SRAM, clears the full SDK-defined
32-KiB DMA arena `[0x30000000,0x30008000)` in 8192 word stores, sets VTOR and
target vector/register state, and branches. It cannot safely return after
irreversible teardown; a terminal failure waits rather than pretending the
selector UI can retry.

The supplied two images are SRAM-linked and leave bootloader services
available across reset. This is not a general loader for arbitrary binaries.
QSPI-linked payloads require a different placement/flash contract and are not
supported. Real cache coherence, in-flight transfers, all reset targets and
electrical behavior require physical validation.

The `support/` headers contain small reusable vector checks and transport-neutral
read-only staging. The backed reader is explicitly synthetic and used by host
tests/development builds only. Default firmware does not use it. There is no
parallel emulator implementation in this repository.
