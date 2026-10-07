# Build/test and USB preparation; module installation remains a manual step.
.PHONY: all build usb build-virtual setup dependencies test check verify-images package package-virtual help
all: build
build: dependencies
	$(MAKE) -C firmware
usb:
	@test -n "$(USB_DIR)" -a -d "$(USB_DIR)" || { echo 'Use make usb USB_DIR=/path/to/mounted/drive'; exit 1; }
	$(MAKE) build check
	python3 scripts/prepare_usb.py --usb "$(USB_DIR)"
build-virtual: dependencies
	$(MAKE) -C firmware VIRTUAL_TRANSPORT=1 EXPERIMENTAL_HANDOFF=1 DMA_ARENA_CLEANUP=1 BUILD_DIR=build-virtual-experimental-dma
setup:
	python3 scripts/setup_dependencies.py
dependencies: setup
	$(MAKE) -C .deps/Aurora-SDK/libs/libDaisy
test:
	python3 -m unittest discover -s tests -v
check: test
	python3 -m compileall -q scripts tests
verify-images: setup
	python3 scripts/verify_images.py "$(FIRMWARE_DIR)"
package:
	python3 scripts/package.py
package-virtual:
	python3 scripts/package.py --virtual
help:
	@printf '%s\n' \
	  'make build          Fetch dependencies and build USB firmware (default)' \
	  'make usb USB_DIR=/path/to/drive  Build, fetch four supported images and copy to USB' \
	  'make build-virtual  Build synthetic-media test firmware; never install it' \
	  'make check          Run host tests and Python syntax checks' \
	  'make package        Rebuild clean committed source into a verified USB bundle' \
	  'make package-virtual  Package a synthetic-media test bundle' \
	  'make verify-images FIRMWARE_DIR=/path/to/files  Check every catalog image'
