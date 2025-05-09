// SPDX-FileCopyrightText: 2025 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/syscall.h>

void fail(void)
{
	sys_dealloc();

	assert(1 == 2);
}

int main(void)
{
	enum ioend endpoint;
	uint8_t available;
	uint8_t in = 0;

	led_set(LED_BLUE);

	config_endpoints(IO_CDC | IO_CH552);

	if (readselect(IO_CDC, &endpoint, &available) < 0) {
		puts(IO_CDC, "can't readselect\r\n");
		// readselect failed! I/O broken? Just redblink.
		assert(1 == 2);
	}

	if (read(IO_CDC, &in, 1, 1) < 0) {
		// read failed! I/O broken? Just redblink.
		puts(IO_CDC, "can't read\r\n");
		assert(1 == 2);
	}

	puts(IO_CDC, "Welcome!\r\n");

	if (sys_alloc() < 0) {
		puts(IO_CDC, "couldn't allocate\r\n");
		assert(1 == 2);
	}

	uint8_t out_data[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

	if (sys_write(0, out_data, sizeof(out_data)) < 0) {
		puts(IO_CDC, "couldn't write\r\n");
		assert(1 == 2);
	}

	uint8_t in_data[14] = {0};

	if (sys_read(0, in_data, sizeof(in_data)) < 0) {
		puts(IO_CDC, "couldn't read\r\n");
		fail();
	}

	if (!memeq(in_data, out_data, sizeof(in_data))) {
		puts(IO_CDC, "read data not same as written\r\n");
		fail();
	}

	if (sys_erase(0, 4096) < 0) {
		puts(IO_CDC, "couldn't erase\r\n");
		fail();
	}

	if (sys_read(0, in_data, sizeof(in_data)) != 0) {
		puts(IO_CDC, "couldn't read erased data\r\n");
		fail();
	}

	uint8_t check_data[sizeof(in_data)] = {0xff, 0xff, 0xff, 0xff, 0xff,
					       0xff, 0xff, 0xff, 0xff, 0xff,
					       0xff, 0xff, 0xff, 0xff};

	if (!memeq(in_data, check_data, sizeof(check_data))) {
		puts(IO_CDC, "erased data not as expected\r\n");
		fail();
	}

	if (sys_dealloc() < 0) {
		puts(IO_CDC, "erased deallocate storage\r\n");
		assert(1 == 2);
	}

	led_set(LED_GREEN);

loop:
	goto loop;
}
