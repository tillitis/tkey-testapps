// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <tkey/assert.h>
#include <tkey/led.h>
#include <tkey/proto.h>
#include <tkey/tk1_mem.h>

#define MODE_TKEYCTRL 0x20
#define MODE_CDC 0x40
#define MODE_HID 0x80

#define FORWARD_TO_DEBUG_HID 0

// clang-format off
static volatile uint32_t *cpu_mon_ctrl  = (volatile uint32_t *) TK1_MMIO_TK1_CPU_MON_CTRL;
static volatile uint32_t *cpu_mon_first = (volatile uint32_t *) TK1_MMIO_TK1_CPU_MON_FIRST;
static volatile uint32_t *cpu_mon_last  = (volatile uint32_t *) TK1_MMIO_TK1_CPU_MON_LAST;
static volatile uint32_t *app_addr      = (volatile uint32_t *) TK1_MMIO_TK1_APP_ADDR;
static volatile uint32_t *app_size      = (volatile uint32_t *) TK1_MMIO_TK1_APP_SIZE;
// clang-format on

int main(void)
{
	uint8_t mode = 0;
	uint8_t len = 0;
	uint8_t data[256];

	// Use Execution Monitor on RAM after app
	*cpu_mon_first = *app_addr + *app_size;
	*cpu_mon_last = TK1_RAM_BASE + TK1_RAM_SIZE;
	*cpu_mon_ctrl = 1;

	led_set(LED_BLUE);

	for (;;) {
		mode = readbyte();
		len = readbyte();

		for (int i = 0; i < len; i++) {
			data[i] = readbyte();
		}

		switch (mode) {
		case MODE_CDC:
			break;
		case MODE_HID:
			break;
		default:
			assert(1 == 2);
		}

		// Echo data
		writebyte(mode);
		writebyte(len);
		for (int i = 0; i < len; i++) {
			writebyte(data[i]);
		}

#if FORWARD_TO_DEBUG_HID
		// Write data to debug end point
		writebyte(MODE_TKEYCTRL);
		writebyte(len);
		for (int i = 0; i < len; i++) {
			writebyte(data[i]);
		}
#endif
	}
}
