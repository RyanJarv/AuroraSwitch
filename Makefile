# Build/test and offline preparer export; module installation remains manual.
.PHONY: all build setup dependencies test test-sanitize check verify-images package preparer fata-ram-probe help reference-html
all: build
reference-html:
	python3 scripts/render_reference.py
build: dependencies
	$(MAKE) -C firmware BUILD_DIR=build-experimental-dma
preparer:
	python3 scripts/package.py --preparer
setup:
	python3 scripts/setup_dependencies.py
dependencies: setup
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
	@printf '%s\n' 'make check' 'make build' 'make package' 'make preparer'
