// Copyright (C) 2022-2024 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

	.section ".text.init"
	.global _start

_start:
        li a0, 0xff000024 # LED MMIO
        li a1, 0x1
loop:
        sw a1, 0(a0)
        addi a1, a1, 1

        li a2, 0
        li a3, 100000        
delay:
        addi a2, a2, 1
        blt a2, a3, delay
        j loop
