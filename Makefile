BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
USER_SHELL_ELF := $(BUILD_DIR)/user/shell.elf
USER_SHELL_BIN := $(BUILD_DIR)/user/shell.bin
USER_SHELL_OBJ := $(BUILD_DIR)/user/shell_bin.o
OPENSBI_RV32 := /usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin

ifneq ($(shell command -v riscv64-unknown-elf-gcc 2>/dev/null),)
CROSS_COMPILE ?= riscv64-unknown-elf-
else
CROSS_COMPILE ?= riscv64-linux-gnu-
endif

CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
LD := $(CROSS_COMPILE)ld

ARCH_FLAGS := -march=rv32ima_zicsr_zicntr -mabi=ilp32 -mcmodel=medany
WARN_FLAGS := -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-pic -fno-pie
CFLAGS := $(ARCH_FLAGS) $(WARN_FLAGS) -std=c11 -O2 -g -Iinclude
ASFLAGS := $(ARCH_FLAGS) -g -Iinclude
LDFLAGS := -T kernel/linker.ld -nostdlib -static -no-pie -Wl,--gc-sections -Wl,--build-id=none -Wl,-Map=$(BUILD_DIR)/kernel.map
LDFLAGS_USER := -nostdlib -static -no-pie -Wl,--gc-sections -Wl,--build-id=none -Wl,-Map=$(BUILD_DIR)/user/shell.map

KERNEL_SRCS := \
	kernel/start.S \
	kernel/kernel.c \
	kernel/console.c \
	kernel/initramfs.c \
	kernel/panic.c \
	kernel/sbi.c \
	kernel/timer.c \
	kernel/thread.c \
	kernel/pmm.c \
	kernel/switch.S \
	kernel/syscall.c \
	kernel/trap.c \
	kernel/user.c \
	kernel/vm.c \
	kernel/trap_entry.S

KERNEL_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS)))
KERNEL_OBJS += $(USER_SHELL_OBJ)

.PHONY: all build run test regression clean toolchain

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

$(BUILD_DIR)/user/%.o: user/%.c | $(BUILD_DIR)/.dir
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(USER_SHELL_ELF): $(BUILD_DIR)/user/shell.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/shell.o -o $@

$(USER_SHELL_BIN): $(USER_SHELL_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_SHELL_OBJ): $(USER_SHELL_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(KERNEL_ELF): toolchain $(KERNEL_OBJS) kernel/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS) $(KERNEL_OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

run: build
	qemu-system-riscv32 -machine virt -bios $(OPENSBI_RV32) -nographic -kernel $(KERNEL_ELF)

test: build
	./scripts/run-qemu-smoke.sh $(KERNEL_ELF) $(OPENSBI_RV32)

regression: build
	QEMU_SMOKE_INPUT='help\necho hello\ntouch a\nls\necho hello > a\ncat a\ncat /README\npwd\nclear\nreboot\n' \
	QEMU_SMOKE_EXPECT='commands:;hello;a;ZOS README;/;user: halted cleanly' \
	./scripts/run-qemu-smoke.sh $(KERNEL_ELF) $(OPENSBI_RV32)

clean:
	rm -rf $(BUILD_DIR)
