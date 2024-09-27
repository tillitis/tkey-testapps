// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stddef.h>
#include <stdint.h>
#include <tkey/proto.h>
#include <tkey/qemu_debug.h>

#include "app_proto.h"

// Send reply frame with response status Not OK (NOK==1), shortest length
void appreply_nok(struct frame_header hdr)
{
	writebyte(genhdr(hdr.id, hdr.endpoint, 0x1, LEN_1));
	writebyte(0);
}

void app_reply(struct frame_header hdr, enum appcmd rspcode, void *buf)
{
	size_t nbytes = 0;
	enum cmdlen len = 0; // length covering (rspcode + length of buf)

	switch (rspcode) {
	case RSP_LOAD_APP:
	case RSP_LOAD_APP_DATA:
	case RSP_DELETE_APP:
	case RSP_UNREGISTER_MGMT_APP:
	case RSP_REGISTER_MGMT_APP:
	case RSP_LOAD_APP_FLASH:
		len = LEN_4;
		nbytes = 4;
		break;

	case RSP_NAME_VERSION:
		len = LEN_32;
		nbytes = 32;
		break;

	case RSP_LOAD_APP_DATA_READY:
		len = LEN_128;
		nbytes = 128;
		break;

	default:
		qemu_puts("app_reply(): Unknown response code: 0x");
		qemu_puthex(rspcode);
		qemu_lf();
		return;
	}

	// Frame Protocol Header
	writebyte(genhdr(hdr.id, hdr.endpoint, 0x0, len));

	// FW protocol header
	writebyte(rspcode);
	nbytes--;

	write(buf, nbytes);
}
