// Copyright (C) 2023-2024 - Tillitis AB
// SPDX-License-Identifier: BSD-2-Clause

// Test of the execution monitor. Should blink green 5 times, then the
// CPU should trap and start blinking red.
//
// If it starts blinking white it was possible to change the exe
// monitor address. If it starts blinking green again after blinking
// white it was possible to turn it off.

#include <stdint.h>
#include <tkey/lib.h>
#include <tkey/tk1_mem.h>

// clang-format off
volatile uint32_t *cpu_mon_ctrl  = (volatile uint32_t *)TK1_MMIO_TK1_CPU_MON_CTRL;
volatile uint32_t *cpu_mon_first = (volatile uint32_t *)TK1_MMIO_TK1_CPU_MON_FIRST;
volatile uint32_t *cpu_mon_last  = (volatile uint32_t *)TK1_MMIO_TK1_CPU_MON_LAST;
static volatile uint32_t *led    = (volatile uint32_t *)TK1_MMIO_TK1_LED;

#define LED_RED   (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE  (1 << TK1_MMIO_TK1_LED_B_BIT)
#define LED_WHITE (LED_RED | LED_GREEN | LED_BLUE)
// clang-format on

int main(void)
{
	int led_on, on_counter;

	// Blink green
	for (led_on = 0, on_counter = 0; on_counter < 5;) {
		*led = led_on ? LED_GREEN : 0;
		if (led_on) {
			on_counter++;
		}
		for (volatile int j = 0; j < 350000; j++) {
		}
		led_on = !led_on;
	}

	*cpu_mon_first = (uint32_t)(&&blinkwhite);
	*cpu_mon_last = (uint32_t)(&&blinkwhite + 1024);

	// Turn on
	*cpu_mon_ctrl = 1;

	// Try changing address - SHOULD NOT WORK
	*cpu_mon_first = (uint32_t)(&&blinkgreen);
	*cpu_mon_last = (uint32_t)(&&blinkgreen + 1024);

	// Turn off - SHOULD NOT WORK
	*cpu_mon_ctrl = 1;

blinkwhite:
	for (led_on = 0, on_counter = 0; on_counter < 10;) {
		*led = led_on ? LED_WHITE : 0;
		if (led_on) {
			on_counter++;
		}
		for (volatile int j = 0; j < 350000; j++) {
		}
		led_on = !led_on;
	}

blinkgreen:
	for (led_on = 0, on_counter = 0; on_counter < 10;) {
		*led = led_on ? LED_GREEN : 0;
		if (led_on) {
			on_counter++;
		}
		for (volatile int j = 0; j < 350000; j++) {
		}
		led_on = !led_on;
	}

	goto blinkwhite;
}
