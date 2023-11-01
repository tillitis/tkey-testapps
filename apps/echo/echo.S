// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

	.section ".text.init"
	.global _start

_start:

        li a0, 0xc3000080 # rx status
        li a1, 0xc3000084 # rx data
        li a2, 0xc3000100 # tx status
        li a3, 0xc3000104 # tx data

read:
        # ready to rx?
        lw a4, 0(a0)
        beq a4,zero,read
write:
        # ready to tx?
        lw a4, 0(a2)
        beq a4,zero,write

        # rx to tx
        lw a4, 0(a1)
        sw a4, 0(a3)

        j read
