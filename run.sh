#!/usr/bin/env bash

KVM=""
CPU="qemu64"
ARCH="x86_64"

# Defaults
MEM="512M"
AMD=0

# Errors and logs
IN_ASM_ERR=""
INT_ERR=""
CPU_RST_ERR=""
GUEST_ERR=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -kvm|--kvm)
            KVM="-enable-kvm"
            shift
            ;;
        -le|--low-end)
            if [[ "$AMD" == "1" ]]; then
                CPU="athlon"
            else
                CPU="pentium2"
            fi
            MEM="256M"
            shift
            ;;
        -he|--high-end)
            if [[ "$AMD" == "1" ]]; then
                CPU="EPYC"
            else
                CPU="Skylake-Client"
            fi
            MEM="2G"
            shift
            ;;
        --amd)
            AMD=1
            shift
            ;;
        -arch|--arch|--architecture)
            ARCH=$2
            shift 2
            ;;
        --show_asm)
            IN_ASM_ERR="in_asm"
            shift
            ;;
        --guest_errors)
            GUEST_ERR="guest_errors"
            shift
            ;;
        --int_logs)
            INT_ERR="int"
            shift
            ;;
        --cpu_reset_logs)
            CPU_RST_ERR="cpu_reset"
            shift
            ;;
        *)
            shift
            ;;
    esac
done

FIRMWARE="bin/${ARCH}/pfi.fd"

[ -f "$FIRMWARE" ] || { echo "Firmware not found: $FIRMWARE"; exit 1; }

if [[ "$ARCH" == "x86_64" ]]; then
    QEMU="qemu-system-x86_64"
elif [[ "$ARCH" == "ia32" ]]; then
    QEMU="qemu-system-i386"
elif [[ "$ARCH" == "aarch64" ]]; then
    QEMU="qemu-system-aarch64"
else
    QEMU="qemu-system-x86_64"
fi

# Initialize empty array
QEMU_LOGS=()

# Add flags conditionally
[[ "$IN_ASM_ERR" != "" ]] && QEMU_LOGS+=("in_asm")
[[ "$INT_ERR" != "" ]] && QEMU_LOGS+=("int")
[[ "$CPU_RST_ERR" != "" ]] && QEMU_LOGS+=("cpu_reset")
[[ "$GUEST_ERR" != "" ]] && QEMU_LOGS+=("guest_errors")

# Join array into comma-separated string
LOGS=$(IFS=, ; echo "${QEMU_LOGS[*]}")

$QEMU \
    $KVM \
    -cpu $CPU \
    -m $MEM \
    -bios $FIRMWARE \
    -vga std \
    -d $QEMU_LOGS \
    -serial stdio \
    -no-reboot