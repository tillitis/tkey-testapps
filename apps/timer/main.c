// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/debug.h>
#include <tkey/lib.h>
#include <tkey/proto.h>
#include <tkey/tk1_mem.h>

#include "app_proto.h"

// clang-format off
volatile uint32_t *led =             (volatile uint32_t *)TK1_MMIO_TK1_LED;
volatile uint32_t *timer =           (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
volatile uint32_t *timer_prescaler = (volatile uint32_t *)TK1_MMIO_TIMER_PRESCALER;
volatile uint32_t *timer_status =    (volatile uint32_t *)TK1_MMIO_TIMER_STATUS;
volatile uint32_t *timer_ctrl =      (volatile uint32_t *)TK1_MMIO_TIMER_CTRL;

#define LED_BLACK  0
#define LED_RED    (1 << TK1_MMIO_TK1_LED_R_BIT)
#define LED_GREEN  (1 << TK1_MMIO_TK1_LED_G_BIT)
#define LED_BLUE   (1 << TK1_MMIO_TK1_LED_B_BIT)
// clang-format on

// Incoming packet from client
struct packet {
	struct frame_header hdr;      // Framing Protocol header
	uint8_t cmd[CMDLEN_MAXBYTES]; // Application level protocol
};

enum cmd_state {
	cmd_framehead,
	cmd_loading,
	cmd_finished,
};

// read_command takes a frame header and a command to fill in after
// parsing. It returns 0 on success.
static int read_command(struct frame_header *hdr, uint8_t *cmd)
{
	uint8_t in = 0;
	uint8_t available = 0;
	enum cmd_state state = cmd_framehead;
	enum ioend endpoint = IO_NONE;

	memset(hdr, 0, sizeof(struct frame_header));
	memset(cmd, 0, CMDLEN_MAXBYTES);

	uint8_t n = 0;
	for (;;) {
		if (readselect(IO_CDC, &endpoint, &available) < 0) {
			debug_puts("readselect errror");
			return -1;
		}

		debug_puts("rc state: ");
		debug_putinthex(state);
		debug_lf();

		switch (state) {
		case cmd_framehead:
			if (read(IO_CDC, &in, 1, 1) < 0) {
				return -1;
			}

			if (parseframe(in, hdr) == -1) {
				debug_puts("Couldn't parse header\n");
				return -1;
			}

			state = cmd_loading;
			break;

		case cmd_loading:
			// Read as much as is available of what we expect from
			// the frame.
			available = available > hdr->len ? hdr->len : available;

			debug_puts("reading ");
			debug_putinthex(available);
			debug_lf();

			int nbytes =
			    read(IO_CDC, &cmd[n], CMDLEN_MAXBYTES, available);
			if (nbytes < 0) {
				debug_puts("read: buffer overrun\n");

				return -1;
			}

			n += nbytes;

			if (n == hdr->len) {
				debug_puts("got them all\n");
				goto finished;
			}
			break;

		default:
			// Shouldn't happen!
			assert(1 == 2);
			break;
		}
	}

finished:
	// Well-behaved apps are supposed to check for a client
	// attempting to probe for firmware. In that case destination
	// is firmware and we just reply NOK.
	if (hdr->endpoint == DST_FW) {
		appreply_nok(*hdr);
		debug_puts("Responded NOK to message meant for fw\n");
		cmd[0] = APP_CMD_FW_PROBE;

		return 0;
	}

	// Is it for us?
	if (hdr->endpoint != DST_SW) {
		debug_puts("Message not meant for app. endpoint was 0x");
		debug_puthex(hdr->endpoint);
		debug_lf();

		return -1;
	}

	return 0;
}

int main(void)
{
	struct packet pkt = {0};
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};

	*led = LED_RED | LED_GREEN;
	for (;;) {
		if (read_command(&pkt.hdr, pkt.cmd) != 0) {
			debug_puts("read_command returned != 0!\n");
			assert(1 == 2);
		}

		// Reset response buffer
		memset(rsp, 0, CMDLEN_MAXBYTES);

		switch (pkt.cmd[0]) {
		case APP_CMD_SET_TIMER:
			debug_puts("APP_CMD_SET_TIMER\n");
			if (pkt.hdr.len != 32) {
				// Bad length
				debug_puts("APP_CMD_SET_TIMER bad length\n");
				rsp[0] = STATUS_BAD;
				appreply(pkt.hdr, APP_RSP_SET_TIMER, rsp);
				break;
			}

			*timer = pkt.cmd[1] + (pkt.cmd[2] << 8) +
				 (pkt.cmd[3] << 16) + (pkt.cmd[4] << 24);

			rsp[0] = STATUS_OK;
			appreply(pkt.hdr, APP_RSP_SET_TIMER, rsp);
			break;

		case APP_CMD_SET_PRESCALER:
			debug_puts("APP_CMD_SET_PRESCALER\n");
			if (pkt.hdr.len != 32) {
				// Bad length
				debug_puts("APP_CMD_SET_TIMER bad length\n");
				rsp[0] = STATUS_BAD;
				appreply(pkt.hdr, APP_RSP_SET_PRESCALER, rsp);
				break;
			}

			*timer_prescaler = pkt.cmd[1] + (pkt.cmd[2] << 8) +
					   (pkt.cmd[3] << 16) +
					   (pkt.cmd[4] << 24);

			rsp[0] = STATUS_OK;
			appreply(pkt.hdr, APP_RSP_SET_PRESCALER, rsp);
			break;

		case APP_CMD_START_TIMER:
			*timer_ctrl = (1 << TK1_MMIO_TIMER_CTRL_START_BIT);

			// Wait for the timer to expire
			while (*timer_status &
			       (1 << TK1_MMIO_TIMER_STATUS_RUNNING_BIT)) {
			}

			rsp[0] = STATUS_OK;
			appreply(pkt.hdr, APP_RSP_START_TIMER, rsp);
			break;

		case APP_CMD_FW_PROBE:
			// Just a probe. Continue.
			break;

		default:
			debug_puts("Received unknown command: ");
			debug_puthex(pkt.cmd[0]);
			debug_lf();
			appreply(pkt.hdr, APP_RSP_UNKNOWN_CMD, rsp);
			break;
		}
	}
}
