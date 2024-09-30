// Copyright (C) 2022-2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/proto.h>
#include <tkey/qemu_debug.h>
#include <tkey/tk1_mem.h>

#define TK1_MMIO_TK1_SYSTEM_RESET 0xff0001C0

// clang-format off
static volatile uint32_t *const sys_reset = (volatile uint32_t *)TK1_MMIO_TK1_SYSTEM_RESET;
// clang-format on

typedef struct {
	uint8_t syscall_no;
	uint32_t offset;
	uint8_t *data;
	size_t size;
	uint32_t *ctx;
} syscall_t;

enum syscall_cmd {
	BLAKE2S = 0,
	ALLOC_AREA,
	DEALLOC_AREA,
	WRITE_DATA,
	READ_DATA,
	ERASE_DATA,
	PRELOAD_STORE,
	PRELOAD_DELETE,
	MGMT_APP_REGISTER,
	MGMT_APP_UNREGISTER,
};

typedef int (*fw_syscall_p)(syscall_t *ctx);

int syscall(syscall_t *ctx)
{
	fw_syscall_p const fw_syscall =
	    (fw_syscall_p) * (volatile uint32_t *)TK1_MMIO_TK1_BLAKE2S;

	return fw_syscall(ctx);
}

bool alloc_area()
{
	/* check if we have an area, otherwise allocate one */
	syscall_t sys_ctx;
	sys_ctx.syscall_no = ALLOC_AREA;
	int ret = syscall(&sys_ctx);

	qemu_puts("ALLOC_AREA: ");
	qemu_putinthex(ret);
	qemu_lf();

	return ret;
}

bool dealloc_area()
{
	/* check if we have an area, otherwise allocate one */
	syscall_t sys_ctx;
	sys_ctx.syscall_no = DEALLOC_AREA;
	bool ret = syscall(&sys_ctx);

	qemu_puts("DEALLOC_AREA: ");
	qemu_putinthex(ret);
	qemu_lf();

	return ret;
}

int write_data(uint32_t offset, uint8_t *data, size_t size)
{
	syscall_t sys_ctx;
	sys_ctx.syscall_no = WRITE_DATA;
	sys_ctx.data = data;
	sys_ctx.size = size;
	sys_ctx.offset = offset;
	int ret = syscall(&sys_ctx);

	qemu_puts("WRITE_DATA: ");
	qemu_putinthex(ret);
	qemu_lf();

	return ret;
}

int read_data(uint32_t offset, uint8_t *data, size_t size)
{
	syscall_t sys_ctx;
	sys_ctx.syscall_no = READ_DATA;
	sys_ctx.data = data;
	sys_ctx.size = size;
	sys_ctx.offset = offset;
	int ret_value = syscall(&sys_ctx);

	qemu_puts("READ_DATA: ");
	qemu_putinthex(ret_value);
	qemu_lf();

	return ret_value;
}

int erase_data(uint32_t offset, size_t size)
{
	syscall_t sys_ctx;
	sys_ctx.syscall_no = ERASE_DATA;
	sys_ctx.size = size;
	sys_ctx.offset = offset;
	int ret_value = syscall(&sys_ctx);

	qemu_puts("ERASE_DATA: ");
	qemu_putinthex(ret_value);
	qemu_lf();

	return ret_value;
}

int main(void)
{
	qemu_puts("Persistent app\n");
	qemu_puts("Waiting for any byte to start..\n");

	led_set(LED_BLUE);
	readbyte();
	led_set(LED_GREEN);

	/* See if we have an area */
	if (!alloc_area()) {
		qemu_puts("No area left\n");
		assert(1 == 2);
	}

	uint8_t tx_buf[4096] = {0x00};

	for (uint16_t i = 0; i < sizeof(tx_buf); i++) {
		tx_buf[i] = i & 0xff;
	}

	led_set(LED_RED);
	for (size_t i = 0; i < 0x20000; i += 0x1000) {
		if (erase_data(i, sizeof(tx_buf))) {
			qemu_puts("erase failed at: \n");
			qemu_putinthex(i);
			assert(1 == 2);
		}
	}

	for (size_t i = 0; i < 0x20000; i += 0x1000) {
		if (write_data(i, tx_buf, sizeof(tx_buf))) {
			qemu_puts("write failed at: \n");
			qemu_putinthex(i);
			assert(1 == 2);
		}
	}

	led_set(LED_GREEN);

	uint8_t rx_buf[4096] = {0x00};
	read_data(0x00, rx_buf, sizeof(tx_buf));

	if (!memeq(rx_buf, tx_buf, sizeof(tx_buf))) {
		qemu_puts("memory not equal at 0x00\n");
		assert(1 == 2);
	}
	memset(rx_buf, 0x00, sizeof(rx_buf));

	read_data(0x10000, rx_buf, sizeof(tx_buf));

	if (!memeq(rx_buf, tx_buf, sizeof(tx_buf))) {
		qemu_puts("memory not equal at 0x10000\n");
		assert(1 == 2);
	}

	qemu_puts("Erase? Send any byte..\n");
	readbyte();

	for (size_t i = 0; i < 0x20000; i += 0x1000) {
		if (erase_data(i, sizeof(tx_buf))) {
			qemu_puts("erase failed at: \n");
			qemu_putinthex(i);
			assert(1 == 2);
		}
	}


	qemu_puts("Deallocate? Send any byte..\n");
	readbyte();

	led_set(LED_RED);
	if (!dealloc_area()) {
		qemu_puts("Deallocate failed\n");
		assert(1 == 2);
	}
	led_set(LED_GREEN);

	qemu_puts("Reset? Send any byte..\n");
	readbyte();

	*sys_reset = 1;

	/* Just spin forever */
	for (;;) {
	}
}
