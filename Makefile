.PHONY: all build build-virtual setup dependencies test check verify-images package package-virtual help
all: build
build: dependencies
	$(MAKE) -C firmware
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
	  'make build-virtual  Build synthetic-media test firmware; never install it' \
	  'make check          Run host tests and Python syntax checks' \
	  'make package        Rebuild clean committed source into a verified USB bundle' \
	  'make package-virtual  Package a synthetic-media test bundle' \
	  'make verify-images FIRMWARE_DIR=/path/to/files  Check every catalog image'
