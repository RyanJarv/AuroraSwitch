# Compilation only. Deliberately no local programming targets or bootloader BIN.
VIRTUAL_TRANSPORT ?= 0
ifeq ($(filter $(VIRTUAL_TRANSPORT),0 1),)
$(error VIRTUAL_TRANSPORT must be 0 or 1)
endif
ifeq ($(VIRTUAL_TRANSPORT),1)
TARGET = AuroraSwitchVirtual
EXPECTED_BUILD_DIR = build-virtual-experimental-dma
C_DEFS += -DSELECTOR_VIRTUAL_TRANSPORT
else
TARGET = AuroraSwitch
EXPECTED_BUILD_DIR = build-experimental-dma
endif
BUILD_DIR ?= $(EXPECTED_BUILD_DIR)
# Keep real/synthetic objects isolated; upstream assigns BUILD_DIR itself.
override BUILD_DIR := $(BUILD_DIR)
ifneq ($(BUILD_DIR),$(EXPECTED_BUILD_DIR))
$(error Use BUILD_DIR=$(EXPECTED_BUILD_DIR) for this transport)
endif
# Preserve link order for stable exact-image diagnostic bindings.
CPP_SOURCES = handoff.cpp selector.cpp
ASM_SOURCES = copy_jump.s
override APP_TYPE := BOOT_SRAM
USE_FATFS = 1
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
ifneq ($(filter program program-dfu program-boot,$(MAKECMDGOALS)),)
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
LDFLAGS += -Tstaging_reserve.ld -Ttrampoline_reserve.ld
$(BUILD_DIR)/$(TARGET).elf: staging_reserve.ld trampoline_reserve.ld
all: check-pins
