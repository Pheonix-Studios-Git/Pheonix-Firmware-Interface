# Pheonix Studios Makefile for PFI (Pheonix Firmware Interface)

# Basic Config
ARCH ?= x86_64
VERBOSE ?= 0
BUILD_DIR = build/$(ARCH)
BIN_DIR = bin/$(ARCH)
SRC_DIR = src/$(ARCH)/src
INC_DIR = src/$(ARCH)/inc
COMMON_INC_DIR = src/common/inc

SCRIPTS_DIR = scripts

TARGET = $(BIN_DIR)/pfi.fd
TARGET_BIN = $(BIN_DIR)/pfi.bin
TARGET_ELF = $(BIN_DIR)/pfi.elf

ifeq ($(ARCH),x86_64)
    CROSS = 
    CFLAGS += -m64 -mcmodel=large
endif

ifeq ($(ARCH),ia32)
    CROSS = i686-linux-gnu-
    CFLAGS += -m32
endif

ifeq ($(ARCH),aarch64)
    CROSS = aarch64-linux-gnu-
endif

ifeq ($(ARCH),arm)
    CROSS = arm-none-eabi-
endif

CC = $(CROSS)gcc
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
NASM = nasm

CFLAGS += -ffreestanding \
		  -fno-exceptions -fno-unwind-tables \
		  -fno-asynchronous-unwind-tables \
		  -fno-stack-protector -fno-pic -fno-pie \
		  -Wall -Wextra -c \
		  -MMD -MP \
		  -I $(COMMON_INC_DIR) -I $(INC_DIR)
LDFLAGS = -nostdlib -T $(SCRIPTS_DIR)/linker.$(ARCH).ld \
          --no-relax \
          --no-check-sections \
          -static \
          -z max-page-size=0x1000 \
          --nmagic \
		  -N

NASM_FLAGS = -f elf64
ENTRY_NASM_FLAGS = -f bin

# Sources
SRCS := $(shell find $(SRC_DIR) -name "*.c")
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

SRCS_ASM := $(shell find $(SRC_DIR) -name "*.S")
OBJS_ASM := $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.S.o,$(SRCS_ASM))

ENTRY_SRCS_NASM := $(shell find $(SRC_DIR) -name "entry.asm" -print -quit)
ENTRY_OBJS_NASM := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.bin,$(ENTRY_SRCS_NASM))

SRCS_NASM := $(filter-out $(ENTRY_SRCS_NASM),$(shell find $(SRC_DIR) -name "*.asm"))
OBJS_NASM := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.asm.o,$(SRCS_NASM))

DEPS := $(OBJS:.o=.d) $(OBJS_ASM:.S.o=.S.d) $(OBJS_NASM:.asm.o=.asm.d) $(ENTRY_OBJS_NASM:.bin=.bin.d)
-include $(DEPS)

# Colors
COLOR_RESET := \033[0m
COLOR_RED := \033[31m
COLOR_GREEN := \033[32m
COLOR_YELLOW := \033[33m
COLOR_BLUE := \033[34m
COLOR_CYAN := \033[36m

define log
	echo -e "$(COLOR_CYAN)[PFI]$(COLOR_RESET) $(1)"
endef

define success
	echo -e "$(COLOR_GREEN)[OK]$(COLOR_RESET) $(1)"
endef

define error
	echo -e "$(COLOR_RED)[ERROR]$(COLOR_RESET) $(1)"
endef

define warn
	echo -e "$(COLOR_YELLOW)[WARN]$(COLOR_RESET) $(1)"
endef

# Rules
all: $(BIN_DIR) $(BUILD_DIR) $(TARGET)
	@$(call success,"Build complete for $(ARCH) at $(TARGET)")

$(BUILD_DIR):
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call log,"Creating build directory"); \
	fi
	@mkdir -p $(BUILD_DIR)
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call success,"Created build directory"); \
	fi

$(BIN_DIR):
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call log,"Creating bin directory"); \
	fi
	@mkdir -p $(BIN_DIR)
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call success,"Created bin directory"); \
	fi

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call log,"Compiling C: $<"); \
	else \
		$(call log,"Compiling C file...."); \
	fi

	@$(CC) $(CFLAGS) -c $< -o $@

	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call success,"Compiled C: $@"); \
	else \
		$(call success,"Compiled C file"); \
	fi

$(BUILD_DIR)/%.S.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call log,"Assembling GAS Assembly: $<"); \
	else \
		$(call log,"Assembling GAS Assembly file..."); \
	fi

	@$(CC) $(CFLAGS) -c $< -o $@
	
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call success,"Assembled GAS Assembly: $@"); \
	else \
		$(call success,"Assembled GAS Assembly file"); \
	fi

$(BUILD_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)

	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call log,"Assembling NASM Assembly: $<"); \
	else \
		$(call log,"Assembling NASM Assembly file..."); \
	fi

	@$(NASM) $(NASM_FLAGS) $< -o $@

	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call success,"Assembled NASM Assembly: $@"); \
	else \
		$(call success,"Assembled NASM Assembly file"); \
	fi
	

$(ENTRY_OBJS_NASM): $(ENTRY_SRCS_NASM)
	@mkdir -p $(dir $@)
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call log,"Assembling Entry NASM Assembly: $<"); \
	else \
		$(call log,"Assembling Entry NASM Assembly file..."); \
	fi

	@$(NASM) $(ENTRY_NASM_FLAGS) $(ENTRY_SRCS_NASM) -o $@

	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call success,"Assembled Entry NASM Assembly: $@"); \
	else \
		$(call success,"Assembled Entry NASM Assembly file"); \
	fi

$(TARGET_ELF): $(OBJS) $(OBJS_ASM) $(OBJS_NASM)
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call log,"Linking $(OBJS_ASM) $(OBJS) $(OBJS_NASM)"); \
	else \
		$(call log,"Linking target..."); \
	fi
	@$(LD) $(LDFLAGS) -o $(TARGET_ELF) $(OBJS_ASM) $(OBJS_NASM) $(OBJS)
	
	@if [ "$(VERBOSE)" = "1" ]; then \
		$(call success,"Linked $(TARGET_ELF)"); \
	else \
		$(call success,"Linked target"); \
	fi

$(TARGET): $(TARGET_ELF) $(ENTRY_OBJS_NASM)
	@$(call log,"Creating Flash image $(TARGET)")
	@$(OBJCOPY) -O binary \
		--strip-all \
		-j .text -j .reset -j .data \
		--gap-fill 0xFF \
		--pad-to 0x1000000 \
		$(TARGET_ELF) $(TARGET)
	@dd if=$(ENTRY_OBJS_NASM) of=$(TARGET) seek=0 conv=notrunc bs=1 count=4096
	@SIZE=$$(stat -c%s $(TARGET)); \
	if [ "$$SIZE" -ne 16777216 ]; then \
		printf "$(COLOR_RED)[Error]$(COLOR_RESET) File size is %s, expected 16777216\n" "$$SIZE" >&2; \
		exit 1; \
	fi
	@$(call success,"Flash image $(TARGET) created and aligned.")

clean:
	@$(call log,"Removing $(BUILD_DIR) and $(BIN_DIR)")
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@$(call success,"Removed $(BUILD_DIR) and $(BIN_DIR)")

# Convenience is good boys
run:
	@$(call log,"Running - $(ARCH)")
	@./run.sh -arch=$(ARCH)
	@$(call success,"Finished")

.PHONY: all clean run