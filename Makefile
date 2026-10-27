# Build and test the selector; module installation remains manual.
.PHONY: all build setup dependencies check-toolchain test test-sanitize check verify-images package fata-ram-probe help reference-html dev
# Use the same compiler directory for the SDK and selector sub-makes.
GCC_PATH ?= $(shell dirname "$$(command -v arm-none-eabi-gcc)")
export GCC_PATH
HOST ?= 127.0.0.1
PORT ?= 8000
all: build
dev:
	python3 -m http.server "$(PORT)" --bind "$(HOST)" --directory site
reference-html:
	python3 scripts/render_reference.py
build: dependencies
	$(MAKE) -C firmware BUILD_DIR=build-experimental-dma
setup:
	python3 scripts/setup_dependencies.py
check-toolchain:
	@"$(GCC_PATH)/arm-none-eabi-gcc" --version 2>/dev/null | grep -q '10-2020-q4-major' || { \
		echo 'GNU Arm 10-2020-q4-major required. Put its bin directory first on PATH or set GCC_PATH.' >&2; \
		echo 'Alternatively: sh scripts/docker_build.sh make build' >&2; \
		exit 1; \
	}
dependencies: check-toolchain setup
	$(MAKE) -C .deps/Aurora-SDK/libs/libDaisy
test:
	python3 -m unittest discover -s tests -v
test-sanitize:
	AURORA_HOST_SANITIZERS=1 python3 -m unittest discover -s tests -p test_helpers.py -v
check: test
	python3 -m compileall -q scripts tests
verify-images: setup
	python3 scripts/verify_images.py "$(FIRMWARE_DIR)"
package:
	python3 scripts/package.py
fata-ram-probe:
	python3 scripts/build_fatamorgana.py
help:
	@printf '%s\n' 'make check' 'make build' 'make package' 'make dev (http://127.0.0.1:8000; override HOST/PORT as needed)'
