# Pheonix Studios Makefile for PFI (Pheonix Firmware Interface)

# Basic Config
ARCH ?= x86_64
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

CFLAGS += -ffreestanding \
		  -fno-exceptions -fno-unwind-tables \
		  -fno-asynchronous-unwind-tables \
		  -fno-stack-protector -fno-pic -fno-pie \
		  -Wall -Wextra -c \
		  -I $(COMMON_INC_DIR) -I $(INC_DIR)
LDFLAGS = -nostdlib -T $(SCRIPTS_DIR)/linker.$(ARCH).ld \
          --no-relax \
          --no-check-sections \
          -static \
          -z max-page-size=0x1000 \
          --nmagic \
		  -N

# Sources
SRCS := $(shell find $(SRC_DIR) -name "*.c")
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

SRCS_ASM := $(shell find $(SRC_DIR) -name "*.S")
OBJS_ASM := $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.S.o,$(SRCS_ASM))

# Colors
COLOR_RESET := \033[0m
COLOR_RED := \033[31m
COLOR_GREEN := \033[32m
COLOR_YELLOW := \033[33m
COLOR_BLUE := \033[34m
COLOR_CYAN := \033[36m

define log
	@echo -e "$(COLOR_CYAN)[PFI]$(COLOR_RESET) $(1)"
endef

define success
	@echo -e "$(COLOR_GREEN)[OK]$(COLOR_RESET) $(1)"
endef

define error
	@echo -e "$(COLOR_RED)[ERROR]$(COLOR_RESET) $(1)"
endef

define warn
	@echo -e "$(COLOR_YELLOW)[WARN]$(COLOR_RESET) $(1)"
endef

# Rules
all: $(BIN_DIR) $(BUILD_DIR) $(TARGET)
	$(call success,"Build complete for $(ARCH) at $(TARGET)")

$(BUILD_DIR):
	$(call log,"Creating build directory")
	@mkdir $(BUILD_DIR)
	$(call success,"Created build directory")

$(BIN_DIR):
	$(call log,"Creating bin directory")
	@mkdir $(BIN_DIR)
	$(call success,"Created bin directory")

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(call log,"Compiling C: $<")
	@$(CC) $(CFLAGS) -c $< -o $@
	$(call success,"Compiled C: $@")

$(BUILD_DIR)/%.S.o: $(SRC_DIR)/%.S
	@mkdir -p $(dir $@)
	$(call log,"Assembling S [Assembly]: $<")
	@$(CC) $(CFLAGS) -c $< -o $@
	$(call success,"Assembled S [Assembly]: $@")

$(TARGET_ELF): $(OBJS) $(OBJS_ASM)
	$(call log,"Linking $(OBJS_ASM) $(OBJS)")
	@$(LD) $(LDFLAGS) -o $(TARGET_ELF) $(OBJS_ASM) $(OBJS)
	$(call success,"Linked $(TARGET_ELF)")

$(TARGET): $(TARGET_ELF)
	$(call log,"Creating Flash image $(TARGET)")
	@$(OBJCOPY) -O binary \
		--strip-all \
		-j .text -j .reset \
		--gap-fill 0xFF \
		--pad-to 0x1000000 \
		$(TARGET_ELF) $(TARGET)
	@SIZE=$$(stat -c%s $(TARGET)); \
	if [ "$$SIZE" -ne 16777216 ]; then \
		printf "$(COLOR_RED)[Error]$(COLOR_RESET) File size is %s, expected 16777216\n" "$$SIZE" >&2; \
		exit 1; \
	fi
	$(call success,"Flash image $(TARGET) created and aligned.")

clean:
	$(call log,"Removing $(BUILD_DIR) and $(BIN_DIR)")
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	$(call success,"Removed $(BUILD_DIR) and $(BIN_DIR)")

# Convenience is good boys
run:
	$(call log,"Running - $(ARCH)")
	@./run.sh $(ARCH)
	$(call success,"Finished")

.PHONY: all clean run