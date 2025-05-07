// SPDX-FileCopyrightText: 2022 Tillitis AB <tillitis.se>
// SPDX-License-Identifier: BSD-2-Clause

#include "app_proto.h"
#include <tkey/debug.h>

// Send app reply with frame header, response code, and LEN_X-1 bytes from buf
void appreply(struct frame_header hdr, enum appcmd rspcode, void *buf)
{
	size_t nbytes;
	enum cmdlen len;
	uint8_t frame[1 + 128]; // Frame header + longest response

	switch (rspcode) {
	case APP_RSP_SET_TIMER:
		len = LEN_4;
		nbytes = 4;
		break;

	case APP_RSP_SET_PRESCALER:
		len = LEN_4;
		nbytes = 4;
		break;

	case APP_RSP_START_TIMER:
		len = LEN_4;
		nbytes = 4;
		break;

	case APP_RSP_UNKNOWN_CMD:
		len = LEN_1;
		nbytes = 1;
		break;

	default:
		debug_puts("appreply(): Unknown response code: ");
		debug_puthex(rspcode);
		debug_lf();

		return;
	}

	// Frame Protocol Header
	frame[0] = genhdr(hdr.id, hdr.endpoint, 0x0, len);

	// App protocol header
	frame[1] = rspcode;

	// Copy payload after app protocol header
	memcpy(&frame[2], buf, nbytes - 1);

	write(IO_CDC, frame, 1 + nbytes);
}

// Send reply frame with response status Not OK (NOK==1), shortest length
void appreply_nok(struct frame_header hdr)
{
	uint8_t buf[2];

	buf[0] = genhdr(hdr.id, hdr.endpoint, 0x1, LEN_1);
	buf[1] = 0; // Not used, but smallest payload is 1 byte
	write(IO_CDC, buf, 2);
}
