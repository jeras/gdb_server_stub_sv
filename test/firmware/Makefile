###############################################################################
# firmware World!
#
# Copyright 2025 Iztok Jeras <iztok.jeras@gmail.com>
#
# Licensed under CERN-OHL-P v2 or later
###############################################################################

TOOLCHAIN_PREFIX ?= riscv64-unknown-elf-

CC = $(TOOLCHAIN_PREFIX)gcc
AS = $(TOOLCHAIN_PREFIX)as

RISCV_ARCH?=rv32i$(shell $(AS) -march=rv32i_zicsr --dump-config 2>/dev/null && echo _zicsr)

SOCKET = $(PWD)/gdb_server_stub_socket

#CFLAGS?=-mabi=ilp32 -Os -Wall -Wextra -Wl,-Bstatic,-T,sections.lds,--strip-debug -ffreestanding -nostdlib
CFLAGS?=-mabi=ilp32 -Os -g -Wall -Wextra -Wl,-Bstatic,-T,sections.lds -ffreestanding -nostdlib

firmware.elf: firmware.s vectors.s firmware.c
	$(CC) -march=$(RISCV_ARCH) $(CFLAGS) -o $@ $^

clean:
	rm -rf firmware.elf firmware.hex
