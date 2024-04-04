// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

// rng stream extraction app.
//
// When loaded and started, this app will continiously generate random data
// words and send them to the host as a stream of bytes.

#include <types.h>
#include <lib.h>
#include <tk1_mem.h>

// clang-format off
static volatile uint32_t *can_rx = (volatile uint32_t *)TK1_MMIO_UART_RX_STATUS;
static volatile uint32_t *rx =     (volatile uint32_t *)TK1_MMIO_UART_RX_DATA;
static volatile uint32_t *uart_tx_status = (volatile uint32_t *)TK1_MMIO_UART_TX_STATUS;
static volatile uint32_t *uart_tx_data =   (volatile uint32_t *)TK1_MMIO_UART_TX_DATA;

// clang-format on


void transmit_w32(uint32_t w)
{
	while (!*uart_tx_status) {
	}
	*uart_tx_data = w & 0xff;
	
	while (!*uart_tx_status) {
	}
	*uart_tx_data = w >> 8 & 0xff;

	while (!*uart_tx_status) {
	}
	*uart_tx_data = w >> 16 & 0xff;

	while (!*uart_tx_status) {
	}
	*uart_tx_data = w >> 24;
}

uint8_t readbyte()
{
	for (;;) {
		if (*can_rx) {
			return *rx;
		}
	}
}


int main(void)
{
	readbyte();
	uint32_t *ram = (uint32_t *)(TK1_RAM_BASE);
	// led_set(LED_RED);

	for (uint32_t w = 0; w < TK1_RAM_SIZE / 4; w++) {
		transmit_w32(ram[w]);
	}
	// led_set(LED_GREEN);
}
