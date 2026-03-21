#!/usr/bin/env bash

KVM=""
CPU="qemu64"
ARCH="x86_64"

# Defaults
MEM="512M"
AMD=0


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

$QEMU \
    $KVM \
    -cpu $CPU \
    -m $MEM \
    -drive if=pflash,format=raw,file=$FIRMWARE \
    -vga std \
    -d int,guest_errors,cpu_reset \
    -no-reboot