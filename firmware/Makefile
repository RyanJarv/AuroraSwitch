# Compilation only. Deliberately no local programming targets or bootloader BIN.
TARGET = AuroraSwitchPreview
EXPERIMENTAL_HANDOFF ?= 1
BUILD_DIR ?= build-experimental-dma
# The upstream core Makefile assigns BUILD_DIR unconditionally.
override BUILD_DIR := $(BUILD_DIR)
DMA_ARENA_CLEANUP ?= 1
ifeq ($(filter $(DMA_ARENA_CLEANUP),0 1),)
$(error DMA_ARENA_CLEANUP must be 0 or 1)
endif
ifeq ($(DMA_ARENA_CLEANUP),1)
ifneq ($(EXPERIMENTAL_HANDOFF),1)
$(error DMA arena cleanup requires experimental handoff)
endif
HANDOFF_BUILD_SUFFIX = -dma
endif
ifeq ($(filter $(EXPERIMENTAL_HANDOFF),0 1),)
$(error EXPERIMENTAL_HANDOFF must be 0 or 1)
endif
VIRTUAL_TRANSPORT ?= 0
QSPI_HANDOFF ?= 0
ifeq ($(filter $(QSPI_HANDOFF),0 1),)
$(error QSPI_HANDOFF must be 0 or 1)
endif
ifeq ($(QSPI_HANDOFF),1)
ifneq ($(EXPERIMENTAL_HANDOFF),1)
$(error QSPI handoff requires experimental handoff)
endif
HANDOFF_BUILD_SUFFIX := $(HANDOFF_BUILD_SUFFIX)-qspi
C_DEFS += -DSELECTOR_QSPI_HANDOFF
endif
ifeq ($(filter $(VIRTUAL_TRANSPORT),0 1),)
$(error VIRTUAL_TRANSPORT must be 0 or 1)
endif
ifeq ($(VIRTUAL_TRANSPORT),1)
ifeq ($(EXPERIMENTAL_HANDOFF),0)
ifneq ($(BUILD_DIR),build-virtual)
$(error Virtual transport requires BUILD_DIR=build-virtual)
endif
TARGET = AuroraSwitchVirtualPreview
else
ifneq ($(BUILD_DIR),build-virtual-experimental$(HANDOFF_BUILD_SUFFIX))
$(error Virtual handoff requires BUILD_DIR=build-virtual-experimental$(HANDOFF_BUILD_SUFFIX))
endif
endif
C_DEFS += -DSELECTOR_VIRTUAL_TRANSPORT
endif
ifeq ($(EXPERIMENTAL_HANDOFF),1)
ifeq ($(VIRTUAL_TRANSPORT),0)
ifneq ($(BUILD_DIR),build-experimental$(HANDOFF_BUILD_SUFFIX))
$(error Experimental build requires BUILD_DIR=build-experimental$(HANDOFF_BUILD_SUFFIX) to isolate its objects)
endif
TARGET = AuroraSwitch
else
TARGET = AuroraSwitchVirtual
endif
CPP_SOURCES += handoff.cpp
ASM_SOURCES = copy_jump.s
C_DEFS += -DSELECTOR_EXPERIMENTAL_HANDOFF
endif
override APP_TYPE := BOOT_SRAM
USE_FATFS = 1
CPP_STANDARD = -std=gnu++17
SDK_ROOT ?= ../.deps/Aurora-SDK
MBEDTLS_ROOT ?= ../.deps/mbedtls
LIBDAISY_DIR = $(SDK_ROOT)/libs/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
CPP_SOURCES += selector.cpp
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
ifeq ($(DMA_ARENA_CLEANUP),1)
# Upstream initializes AS_DEFS; append after inclusion, not before it.
AS_DEFS += -DSELECTOR_DMA_ARENA_CLEANUP
endif
ifeq ($(QSPI_HANDOFF),1)
AS_DEFS += -DSELECTOR_QSPI_HANDOFF
endif
LDFLAGS += -Tstaging_reserve.ld
$(BUILD_DIR)/$(TARGET).elf: staging_reserve.ld
ifeq ($(EXPERIMENTAL_HANDOFF),1)
LDFLAGS += -Ttrampoline_reserve.ld
$(BUILD_DIR)/$(TARGET).elf: trampoline_reserve.ld
endif
all: check-pins
