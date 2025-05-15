// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <monocypher/monocypher-ed25519.h>
#include <stdbool.h>
#include <tkey/assert.h>
#include <tkey/led.h>
#include <tkey/lib.h>
#include <tkey/proto.h>
#include <tkey/qemu_debug.h>
#include <tkey/tk1_mem.h>
#include <tkey/touch.h>

// clang-format off
static volatile uint32_t *cdi             = (volatile uint32_t *) TK1_MMIO_TK1_CDI_FIRST;
static volatile uint32_t *timer           = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *timer_prescaler = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
static volatile uint32_t *timer_ctrl      = (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;
// clang-format on

#define MAX_SIGN_SIZE 4096

// Context for the loading of a message
struct context {
	uint8_t secret_key[64]; // Private key. Keep this here below
				// message in memory.
	uint8_t pubkey[32];
	uint8_t message[MAX_SIGN_SIZE];
	uint32_t message_size;
};

void puts(char *s)
{
	for (char *c = s; *c != '\0'; c++) {
		writebyte(*c);
	}
}

void puthex(uint8_t c)
{
	unsigned int upper = (c >> 4) & 0xf;
	unsigned int lower = c & 0xf;
	writebyte(upper < 10 ? '0' + upper : 'A' - 10 + upper);
	writebyte(lower < 10 ? '0' + lower : 'A' - 10 + lower);
}

void putinthex(const uint32_t n)
{
	uint8_t buf[4];

	memcpy(buf, &n, 4);
	for (int i = 3; i > -1; i--) {
		puthex(buf[i]);
	}
	writebyte('\r');
	writebyte('\n');
}

void puthexn(uint8_t *p, int n)
{
	for (int i = 0; i < n; i++) {
		puthex(p[i]);
	}
}

int main(void)
{
	struct context ctx = {0};
	uint8_t signature[64] = {0};
	uint32_t start = 0;
	uint32_t end = 0;

	led_set(LED_BLUE);

	// Generate a public key from CDI
	crypto_ed25519_key_pair(ctx.secret_key, ctx.pubkey, (uint8_t *)cdi);

	memcpy_s(ctx.message, MAX_SIGN_SIZE, "Håll den som en gyro!", 22);
	ctx.message_size = 22;

	// Wait for terminal program and a character to be typed
	readbyte();

	puts("Outputs signing times in hex in milliseconds\r\n");

	// Set prescaler so we count in milliseconds. CPU freq is 18
	// MHz, so this means 18 000.
	*timer_prescaler = 18000;

	// Set timer to some huge value
	*timer = 1000000000;

	// Start timer
	*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);

	for (;;) {
		start = *timer;
		crypto_ed25519_sign(signature, ctx.secret_key, ctx.message,
				    ctx.message_size);
		end = *timer;

		putinthex(start - end);
	}
}
