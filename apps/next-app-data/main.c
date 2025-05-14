// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/io.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/syscall.h>

int main(void)
{
	enum ioend endpoint;
	uint8_t available;
	uint8_t in = 0;

	if (readselect(IO_CDC, &endpoint, &available) < 0) {
		// readselect failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	if (read(IO_CDC, &in, 1, 1) < 0) {
		// read failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	puts(IO_CDC, "next-app-data\r\n");

	uint8_t app_data[RESET_DATA_SIZE];
	uint8_t compare_data[RESET_DATA_SIZE];

	memset(compare_data, 17, RESET_DATA_SIZE);

	// Read the next_app_data
	sys_reset_data(app_data);

	puts(IO_CDC, "app_data: \r\n");
	hexdump(IO_CDC, app_data, RESET_DATA_SIZE);

	// Same as we have?
	if (!memeq(app_data, compare_data, RESET_DATA_SIZE)) {
		assert(1 == 2);
	}

	led_set(LED_GREEN);

loop:
	goto loop;
}
