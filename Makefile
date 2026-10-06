.PHONY: all setup dependencies test verify-images package
all:
	$(MAKE) -C firmware
setup:
	python3 scripts/setup_dependencies.py
dependencies: setup
	$(MAKE) -C .deps/Aurora-SDK/libs/libDaisy
test:
	python3 -m unittest discover -s tests -v
verify-images: setup
	python3 scripts/verify_images.py "$(FIRMWARE_DIR)"
package: all
	python3 scripts/package.py
