#!/usr/bin/env sh
set -eu

kernel="${1:-build/kernel.elf}"
opensbi="${2:-/usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin}"

if ! command -v qemu-system-riscv32 >/dev/null 2>&1; then
    echo "error: qemu-system-riscv32 not found on PATH; cannot run smoke test" >&2
    echo "hint: sudo apt-get install -y qemu-system-misc opensbi" >&2
    exit 1
fi

if [ ! -f "$opensbi" ]; then
    echo "error: OpenSBI RISC-V32 firmware not found; cannot run smoke test" >&2
    echo "missing: $opensbi" >&2
    echo "hint: sudo apt-get install -y opensbi" >&2
    exit 1
fi

if [ ! -f "$kernel" ]; then
    echo "error: kernel ELF not found: $kernel" >&2
    exit 1
fi

tmp="${TMPDIR:-/tmp}/zos-qemu-smoke.$$"
trap 'rm -f "$tmp"' EXIT INT TERM

timeout 8s qemu-system-riscv32 \
    -machine virt \
    -bios "$opensbi" \
    -nographic \
    -kernel "$kernel" >"$tmp" 2>&1 || true

if grep -q "ZOS booting..." "$tmp"; then
    echo "smoke test passed"
else
    echo "error: QEMU boot output did not contain expected banner" >&2
    echo "----- qemu output -----" >&2
    cat "$tmp" >&2
    echo "-----------------------" >&2
    exit 1
fi
