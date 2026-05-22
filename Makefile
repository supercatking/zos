BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
USER_SHELL_ELF := $(BUILD_DIR)/user/shell.elf
USER_SHELL_BIN := $(BUILD_DIR)/user/shell.bin
USER_SHELL_OBJ := $(BUILD_DIR)/user/shell_bin.o
USER_ECHO_ELF := $(BUILD_DIR)/user/bin/echo.elf
USER_ECHO_BIN := $(BUILD_DIR)/user/bin/echo.bin
USER_ECHO_OBJ := $(BUILD_DIR)/user/bin/echo_bin.o
USER_ECHO_ELF_OBJ := $(BUILD_DIR)/user/bin/echo_elf.o
USER_CAT_ELF := $(BUILD_DIR)/user/bin/cat.elf
USER_CAT_BIN := $(BUILD_DIR)/user/bin/cat.bin
USER_CAT_OBJ := $(BUILD_DIR)/user/bin/cat_bin.o
USER_LS_ELF := $(BUILD_DIR)/user/bin/ls.elf
USER_LS_BIN := $(BUILD_DIR)/user/bin/ls.bin
USER_LS_OBJ := $(BUILD_DIR)/user/bin/ls_bin.o
USER_HELP_ELF := $(BUILD_DIR)/user/bin/help.elf
USER_HELP_BIN := $(BUILD_DIR)/user/bin/help.bin
USER_HELP_OBJ := $(BUILD_DIR)/user/bin/help_bin.o
USER_FORKTEST_ELF := $(BUILD_DIR)/user/bin/forktest.elf
USER_FORKTEST_BIN := $(BUILD_DIR)/user/bin/forktest.bin
USER_FORKTEST_OBJ := $(BUILD_DIR)/user/bin/forktest_bin.o
USER_VMTEST_ELF := $(BUILD_DIR)/user/bin/vmtest.elf
USER_VMTEST_BIN := $(BUILD_DIR)/user/bin/vmtest.bin
USER_VMTEST_OBJ := $(BUILD_DIR)/user/bin/vmtest_bin.o
USER_MULTIFORKTEST_ELF := $(BUILD_DIR)/user/bin/multiforktest.elf
USER_MULTIFORKTEST_BIN := $(BUILD_DIR)/user/bin/multiforktest.bin
USER_MULTIFORKTEST_OBJ := $(BUILD_DIR)/user/bin/multiforktest_bin.o
USER_SCHEDTEST_ELF := $(BUILD_DIR)/user/bin/schedtest.elf
USER_SCHEDTEST_BIN := $(BUILD_DIR)/user/bin/schedtest.bin
USER_SCHEDTEST_OBJ := $(BUILD_DIR)/user/bin/schedtest_bin.o
USER_PROGRAM_OBJS := $(USER_SHELL_OBJ) $(USER_ECHO_OBJ) $(USER_ECHO_ELF_OBJ) $(USER_CAT_OBJ) $(USER_LS_OBJ) $(USER_HELP_OBJ) $(USER_FORKTEST_OBJ) $(USER_VMTEST_OBJ) $(USER_MULTIFORKTEST_OBJ) $(USER_SCHEDTEST_OBJ)
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
	kernel/elf.c \
	kernel/syscall.c \
	kernel/trap.c \
	kernel/user.c \
	kernel/vm.c \
	kernel/trap_entry.S

KERNEL_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS)))
KERNEL_OBJS += $(USER_PROGRAM_OBJS)

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

$(USER_ECHO_ELF): $(BUILD_DIR)/user/bin/echo.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/echo.o -o $@

$(USER_ECHO_BIN): $(USER_ECHO_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_ECHO_OBJ): $(USER_ECHO_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_ECHO_ELF_OBJ): $(USER_ECHO_ELF)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_CAT_ELF): $(BUILD_DIR)/user/bin/cat.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/cat.o -o $@

$(USER_CAT_BIN): $(USER_CAT_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_CAT_OBJ): $(USER_CAT_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_LS_ELF): $(BUILD_DIR)/user/bin/ls.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/ls.o -o $@

$(USER_LS_BIN): $(USER_LS_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_LS_OBJ): $(USER_LS_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_HELP_ELF): $(BUILD_DIR)/user/bin/help.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/help.o -o $@

$(USER_HELP_BIN): $(USER_HELP_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_HELP_OBJ): $(USER_HELP_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_FORKTEST_ELF): $(BUILD_DIR)/user/bin/forktest.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/forktest.o -o $@

$(USER_FORKTEST_BIN): $(USER_FORKTEST_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_FORKTEST_OBJ): $(USER_FORKTEST_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_VMTEST_ELF): $(BUILD_DIR)/user/bin/vmtest.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/vmtest.o -o $@

$(USER_VMTEST_BIN): $(USER_VMTEST_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_VMTEST_OBJ): $(USER_VMTEST_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_MULTIFORKTEST_ELF): $(BUILD_DIR)/user/bin/multiforktest.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/multiforktest.o -o $@

$(USER_MULTIFORKTEST_BIN): $(USER_MULTIFORKTEST_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_MULTIFORKTEST_OBJ): $(USER_MULTIFORKTEST_BIN)
	$(LD) -m elf32lriscv -r -b binary -o $@ $<

$(USER_SCHEDTEST_ELF): $(BUILD_DIR)/user/bin/schedtest.o user/linker.ld
	$(CC) $(ARCH_FLAGS) $(LDFLAGS_USER) -T user/linker.ld $(BUILD_DIR)/user/bin/schedtest.o -o $@

$(USER_SCHEDTEST_BIN): $(USER_SCHEDTEST_ELF)
	$(OBJCOPY) -O binary $< $@

$(USER_SCHEDTEST_OBJ): $(USER_SCHEDTEST_BIN)
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
	QEMU_SMOKE_INPUT='help\nwhich echo\n/bin/echo hello\n/bin/elfecho hello\necho hello\n/bin/forktest\n/bin/multiforktest\n/bin/vmtest\n/bin/schedtest\ntouch a\nls /bin\nls\necho hello > a\ncat a\ncat /README\nps\npwd\nclear\nenv\nhistory\ngrep hello a\nwc a\ntrue\nfalse\ncd /\nreboot\n' \
	QEMU_SMOKE_EXPECT='commands:;echo;hello;elfecho;forktest: child saw 0;forktest: wait reaped child;multifork: wait reaped 3;multifork: ok;vmtest: isolation ok;schedtest: wait reaped 3;schedtest: ok;multiforktest;schedtest;a;ZOS README;pid: 1 ppid: 0 state: running name: sh;PATH=/bin;history;lines=1 words=1 bytes=6;user: halted cleanly' \
	./scripts/run-qemu-smoke.sh $(KERNEL_ELF) $(OPENSBI_RV32)

clean:
	rm -rf $(BUILD_DIR)
