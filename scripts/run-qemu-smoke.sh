#!/usr/bin/env sh
set -eu

kernel="${1:-build/kernel.elf}"
opensbi="${2:-/usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin}"
shift 2 2>/dev/null || true

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

expectations="ZOS booting..."
if [ "${QEMU_SMOKE_EXPECT:-}" != "" ]; then
    expectations="$expectations;${QEMU_SMOKE_EXPECT}"
fi
for pattern in "$@"; do
    expectations="$expectations;$pattern"
done

old_ifs="$IFS"
IFS=";"
for pattern in $expectations; do
    [ "$pattern" = "" ] && continue
    if ! grep -q "$pattern" "$tmp"; then
        echo "error: QEMU boot output did not contain expected pattern: $pattern" >&2
        echo "----- qemu output -----" >&2
        cat "$tmp" >&2
        echo "-----------------------" >&2
        IFS="$old_ifs"
        exit 1
    fi
done
IFS="$old_ifs"

echo "smoke test passed"
