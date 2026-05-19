BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
OPENSBI_RV32 := /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin

ifneq ($(shell command -v riscv64-unknown-elf-gcc 2>/dev/null),)
CROSS_COMPILE ?= riscv64-unknown-elf-
else
CROSS_COMPILE ?= riscv64-linux-gnu-
endif

CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

ARCH_FLAGS := -march=rv32ima_zicsr_zicntr -mabi=ilp32 -mcmodel=medany
WARN_FLAGS := -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-pic -fno-pie
CFLAGS := $(ARCH_FLAGS) $(WARN_FLAGS) -std=c11 -O2 -g -Iinclude
ASFLAGS := $(ARCH_FLAGS) -g -Iinclude
LDFLAGS := -T kernel/linker.ld -nostdlib -static -no-pie -Wl,--gc-sections -Wl,--build-id=none -Wl,-Map=$(BUILD_DIR)/kernel.map

KERNEL_SRCS := \
	kernel/start.S \
	kernel/kernel.c \
	kernel/console.c \
	kernel/panic.c \
	kernel/sbi.c \
	kernel/timer.c \
	kernel/thread.c \
	kernel/switch.S \
	kernel/trap.c \
	kernel/trap_entry.S

KERNEL_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS)))

.PHONY: all build run test clean toolchain

all: build

build: $(KERNEL_ELF) $(KERNEL_BIN)

toolchain:
	@command -v $(CC) >/dev/null || { echo "error: $(CC) not found on PATH"; exit 1; }
	@command -v $(OBJCOPY) >/dev/null || { echo "error: $(OBJCOPY) not found on PATH"; exit 1; }

$(BUILD_DIR)/.dir:
	@mkdir -p $(BUILD_DIR)
	@touch $@

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)/.dir
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)/.dir
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): toolchain $(KERNEL_OBJS) kernel/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS) $(KERNEL_OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

run: build
	qemu-system-riscv32 -machine virt -bios $(OPENSBI_RV32) -nographic -kernel $(KERNEL_ELF)

test: build
	./scripts/run-qemu-smoke.sh $(KERNEL_ELF) $(OPENSBI_RV32)

clean:
	rm -rf $(BUILD_DIR)
