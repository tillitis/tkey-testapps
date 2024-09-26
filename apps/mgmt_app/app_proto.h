// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef APP_PROTO_H
#define APP_PROTO_H

#include <tkey/lib.h>
#include <tkey/proto.h>
#

// clang-format off
enum appcmd {
	CMD_NAME_VERSION	= 0x01,
	RSP_NAME_VERSION	= 0x02,
	CMD_LOAD_APP		= 0x03,
	RSP_LOAD_APP		= 0x04,
	CMD_LOAD_APP_DATA	= 0x05,
	RSP_LOAD_APP_DATA	= 0x06,
	RSP_LOAD_APP_DATA_READY	= 0x07,
	CMD_DELETE_APP		= 0x08,
	RSP_DELETE_APP		= 0x09,
	CMD_REGISTER_MGMT_APP	= 0x0a,
	RSP_REGISTER_MGMT_APP	= 0x0b,
	CMD_UNREGISTER_MGMT_APP	= 0x0c,
	RSP_UNREGISTER_MGMT_APP	= 0x0d,

	CMD_LOAD_APP_FLASH	= 0xf0,
	RSP_LOAD_APP_FLASH	= 0xf1,

	CMD_FW_PROBE	    = 0xff,
};
// clang-format on

void appreply_nok(struct frame_header hdr);
void app_reply(struct frame_header hdr, enum appcmd rspcode, void *buf);

#endif
