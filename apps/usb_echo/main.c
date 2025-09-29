// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <tkey/assert.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/tk1_mem.h>

#define FORWARD_TO_DEBUG_HID 0
#if FORWARD_TO_DEBUG_HID
#define ENABLED_EPS (IO_CDC | IO_FIDO | IO_DEBUG)
#else
#define ENABLED_EPS (IO_CDC | IO_FIDO)
#endif

// clang-format off
static volatile uint32_t *cpu_mon_ctrl  = (volatile uint32_t *) TK1_MMIO_TK1_CPU_MON_CTRL;
static volatile uint32_t *cpu_mon_first = (volatile uint32_t *) TK1_MMIO_TK1_CPU_MON_FIRST;
static volatile uint32_t *cpu_mon_last  = (volatile uint32_t *) TK1_MMIO_TK1_CPU_MON_LAST;
static volatile uint32_t *app_addr      = (volatile uint32_t *) TK1_MMIO_TK1_APP_ADDR;
static volatile uint32_t *app_size      = (volatile uint32_t *) TK1_MMIO_TK1_APP_SIZE;
// clang-format on

int main(void)
{
	enum ioend ep = 0;
	uint8_t len = 0;
	uint8_t data[256];
	uint8_t led_color = LED_BLUE;

	config_endpoints(ENABLED_EPS);

	// Use Execution Monitor on RAM after app
	*cpu_mon_first = *app_addr + *app_size;
	*cpu_mon_last = TK1_RAM_BASE + TK1_RAM_SIZE;
	*cpu_mon_ctrl = 1;

	for (;;) {
		led_set(led_color);

		if (led_color != LED_BLUE) {
			led_color = LED_BLUE;
		} else {
			led_color = LED_RED;
		}

		readselect(IO_CDC | IO_FIDO, &ep, &len);
		read(ep, data, sizeof(data), len);

		// Echo data
		write(ep, data, len);

#if FORWARD_TO_DEBUG_HID
		// Write data to debug end point
		write(IO_DEBUG, data, len);
#endif
	}
}
