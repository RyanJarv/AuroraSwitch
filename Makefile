# Build/test and USB preparation; module installation remains a manual step.
.PHONY: all build usb download-release build-virtual setup dependencies test check verify-images package package-virtual fata-ram-probe help
RELEASE_TAG ?= latest
# Explicit development opt-in; releases and USB preparation remain RAM-only.
QSPI_HANDOFF ?= 0
ifeq ($(filter $(QSPI_HANDOFF),0 1),)
$(error QSPI_HANDOFF must be 0 or 1)
endif
QSPI_PACKAGE_OPTION = $(if $(filter 1,$(QSPI_HANDOFF)),--qspi,)
all: build
build: dependencies
	$(MAKE) -C firmware BUILD_DIR=build-experimental-dma$(if $(filter 1,$(QSPI_HANDOFF)),-qspi,)
usb:
	@test -n "$(USB_DIR)" -a -d "$(USB_DIR)" || { echo 'Use make usb USB_DIR=/path/to/mounted/drive'; exit 1; }
	$(MAKE) build check
	python3 scripts/prepare_usb.py --usb "$(USB_DIR)"
download-release:
	@test -n "$(USB_DIR)" -a -d "$(USB_DIR)" || { echo 'Use make download-release USB_DIR=/path/to/mounted/drive'; exit 1; }
	python3 scripts/setup_dependencies.py --verification-only
	python3 scripts/prepare_usb.py --usb "$(USB_DIR)" --release-tag "$(RELEASE_TAG)"
build-virtual: dependencies
	$(MAKE) -C firmware VIRTUAL_TRANSPORT=1 EXPERIMENTAL_HANDOFF=1 DMA_ARENA_CLEANUP=1 BUILD_DIR=build-virtual-experimental-dma$(if $(filter 1,$(QSPI_HANDOFF)),-qspi,)
setup:
	python3 scripts/setup_dependencies.py
dependencies: setup
	$(MAKE) -C .deps/Aurora-SDK/libs/libDaisy
test:
	python3 -m unittest discover -s tests -v
check: test
	python3 -m compileall -q scripts tests
verify-images: setup
	python3 scripts/verify_images.py "$(FIRMWARE_DIR)" $(QSPI_PACKAGE_OPTION)
package:
	python3 scripts/package.py $(QSPI_PACKAGE_OPTION)
package-virtual:
	python3 scripts/package.py --virtual $(QSPI_PACKAGE_OPTION)
fata-ram-probe:
	python3 scripts/build_fatamorgana.py
help:
	@printf '%s\n' \
	  'make build          Fetch dependencies and build USB firmware (default)' \
	  'make usb USB_DIR=/path/to/drive  Build, fetch four supported images and copy to USB' \
	  'make download-release USB_DIR=/path/to/drive  Download latest release and prepare USB (no Arm compiler)' \
	  'make build-virtual  Build synthetic-media test firmware; never install it' \
	  'make check          Run host tests and Python syntax checks' \
	  'make package        Rebuild clean committed source into a verified USB bundle' \
	  'make package-virtual  Package a synthetic-media test bundle' \
	  'QSPI_HANDOFF=1     Opt in to experimental Dirt Verb packaging/verification' \
	  'make verify-images FIRMWARE_DIR=/path/to/files  Check every catalog image'
