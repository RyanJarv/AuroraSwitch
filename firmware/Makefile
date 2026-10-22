# Compilation only. Deliberately no local programming targets or bootloader BIN.
TARGET = AuroraSwitch
BUILD_DIR ?= build-experimental-dma
override BUILD_DIR := $(BUILD_DIR)
# Preserve link order for stable exact-image diagnostic bindings.
CPP_SOURCES = handoff.cpp selector.cpp
ASM_SOURCES = copy_jump.s resident_boot.s
override APP_TYPE := BOOT_SRAM
CPP_STANDARD = -std=gnu++17
SDK_ROOT ?= ../.deps/Aurora-SDK
MBEDTLS_ROOT ?= ../.deps/mbedtls
LIBDAISY_DIR = $(SDK_ROOT)/libs/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
C_SOURCES = $(MBEDTLS_ROOT)/library/sha256.c $(MBEDTLS_ROOT)/library/platform_util.c
C_INCLUDES += -I$(SDK_ROOT)/include -I$(MBEDTLS_ROOT)/include -I.
C_DEFS += -DMBEDTLS_CONFIG_FILE='"sha256_config.h"'
GCC_PATH ?= $(shell dirname "$$(command -v arm-none-eabi-gcc)")

.DEFAULT_GOAL := all
ifneq ($(filter program program-dfu program-boot debug openocd debug_client,$(MAKECMDGOALS)),)
$(error Selector physical deployment is not authorized or qualified)
endif
.PHONY: check-pins
check-pins:
	@test "$$(git -C $(SDK_ROOT) rev-parse HEAD)" = 69b74a88b25e2fb4d722fc269bfd9395dd28edb5
	@test "$$(git -C $(LIBDAISY_DIR) rev-parse HEAD)" = 63fcabd38a20e14bc744499f0460e47925ea753e
	@test "$$(git -C $(MBEDTLS_ROOT) rev-parse HEAD)" = 2fc8413bfcb51354c8e679141b17b3f1a5942561
	@test -z "$$(git -C $(SDK_ROOT) status --porcelain --untracked-files=no)"
	@test -z "$$(git -C $(LIBDAISY_DIR) status --porcelain --untracked-files=no)"
	@test -z "$$(git -C $(MBEDTLS_ROOT) status --porcelain --untracked-files=no)"

include $(SYSTEM_FILES_DIR)/Makefile
# Share numeric definitions with C++ and assembly; ld itself has no C preprocessor.
SELECTOR_LINKER_SCRIPTS = $(addprefix $(BUILD_DIR)/,staging_reserve.ld trampoline_reserve.ld)
LDFLAGS += $(addprefix -T,$(SELECTOR_LINKER_SCRIPTS))
$(SELECTOR_LINKER_SCRIPTS): $(BUILD_DIR)/%.ld: %.ld ../support/selector_constants.h | $(BUILD_DIR)
	$(CC) -E -P -x c $< -o $@
# Upstream assembly rules do not emit header dependencies.
$(addprefix $(BUILD_DIR)/,copy_jump.o resident_boot.o): ../support/selector_constants.h
# The upstream rules omit library/linker inputs and the included build rules.
# Recompile for SDK flag changes; relink for rebuilt archives or memory layouts.
$(OBJECTS): $(SYSTEM_FILES_DIR)/Makefile
$(BUILD_DIR)/$(TARGET).elf: $(LIBDAISY_DIR)/build/libdaisy.a $(LDSCRIPT) $(SELECTOR_LINKER_SCRIPTS)
all: check-pins
