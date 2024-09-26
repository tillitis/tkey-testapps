// Copyright (C) 2022-2024 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tkey/assert.h>
#include <tkey/blake2s.h>
#include <tkey/lib.h>
#include <tkey/proto.h>
#include <tkey/qemu_debug.h>
#include <tkey/tk1_mem.h>

#include "app_proto.h"

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
	PRELOAD_STORE,
	PRELOAD_DELETE,
	MGMT_APP_REGISTER,
	MGMT_APP_UNREGISTER,
};

enum state {
	STATE_INITIAL,
	STATE_LOADING,
	STATE_RUN,
	STATE_FAIL,
	STATE_LOAD_APP_FLASH,
	STATE_MAX,
};

const uint8_t app_name0[4] = "tk1 ";
const uint8_t app_name1[4] = "mgmt";
const uint32_t app_version = 0x00000001;

// Context for the loading of a TKey program
struct context {
	uint32_t left;	    // Bytes left to receive
	uint32_t count;	    // Bytes in buffer before write
	uint32_t offset;    // Offset to write to in flash
	uint8_t digest[32]; // Program digest
	uint8_t *loadaddr;  // Where we are currently loading a TKey program
	bool use_uss;	    // Use USS?
	uint8_t uss[32];    // User Supplied Secret, if any
};

typedef int (*fw_syscall_p)(syscall_t *ctx);

int syscall(syscall_t *ctx)
{
	fw_syscall_p const fw_syscall =
	    (fw_syscall_p) * (volatile uint32_t *)TK1_MMIO_TK1_BLAKE2S;

	return fw_syscall(ctx);
}

bool register_mgmt()
{
	syscall_t sys_ctx;
	sys_ctx.syscall_no = MGMT_APP_REGISTER;
	int ret = syscall(&sys_ctx);

	qemu_puts("MGMT_APP_REGISTER: ");
	qemu_putinthex(ret);
	qemu_lf();

	return ret;
}

bool unregister_mgmt()
{
	syscall_t sys_ctx;
	sys_ctx.syscall_no = MGMT_APP_UNREGISTER;
	bool ret = syscall(&sys_ctx);

	qemu_puts("MGMT_APP_UNREGISTER: ");
	qemu_putinthex(ret);
	qemu_lf();

	return ret;
}

int preload_store(struct context *ctx, uint32_t size)
{
	syscall_t sys_ctx;
	sys_ctx.syscall_no = PRELOAD_STORE;
	sys_ctx.offset = ctx->offset;
	sys_ctx.size = size;
	sys_ctx.data = ctx->loadaddr;

	bool ret = syscall(&sys_ctx);

	ctx->offset += size;

	qemu_puts("PRELOAD_STORE: ");
	qemu_putinthex(sys_ctx.offset);
	qemu_lf();
	qemu_puts("ret: ");
	qemu_putinthex(ret);
	qemu_lf();

	return ret;
}

int preload_delete()
{
	syscall_t sys_ctx;
	sys_ctx.syscall_no = PRELOAD_DELETE;

	bool ret = syscall(&sys_ctx);

	qemu_puts("PRELOAD_DELETE: ");
	qemu_putinthex(ret);
	qemu_lf();

	return ret;
}

static void copy_name(uint8_t *buf, const size_t bufsiz, const uint32_t word)
{
	assert(bufsiz >= 4);

	buf[0] = word >> 24;
	buf[1] = word >> 16;
	buf[2] = word >> 8;
	buf[3] = word;
}

// read_command takes a frame header and a command to fill in after
// parsing. It returns 0 on success.
static int read_command(struct frame_header *hdr, uint8_t *cmd)
{
	uint8_t in = 0;

	memset(hdr, 0, sizeof(struct frame_header));
	memset(cmd, 0, CMDLEN_MAXBYTES);

	in = readbyte();

	if (parseframe(in, hdr) == -1) {
		qemu_puts("Couldn't parse header\n");
		return -1;
	}

	// Now we know the size of the cmd frame, read it all
	if (read(cmd, CMDLEN_MAXBYTES, hdr->len) != 0) {
		qemu_puts("read: buffer overrun\n");
		return -1;
	}

	// Well-behaved apps are supposed to check for a client
	// attempting to probe for firmware. In that case destination
	// is firmware and we just reply NOK.
	if (hdr->endpoint == DST_FW) {
		appreply_nok(*hdr);
		qemu_puts("Responded NOK to message meant for fw\n");
		cmd[0] = CMD_FW_PROBE;
		return 0;
	}

	// Is it for us?
	if (hdr->endpoint != DST_SW) {
		qemu_puts("Message not meant for app. endpoint was 0x");
		qemu_puthex(hdr->endpoint);
		qemu_lf();

		return -1;
	}

	return 0;
}

static enum state initial_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};

	switch (cmd[0]) {
	case CMD_NAME_VERSION:
		qemu_puts("cmd: name-version\n");
		if (hdr->len != 1) {
			// Bad length
			state = STATE_FAIL;
			break;
		}

		copy_name(rsp, CMDLEN_MAXBYTES, (uint32_t)app_name0);
		copy_name(&rsp[4], CMDLEN_MAXBYTES - 4, (uint32_t)app_name1);
		wordcpy_s(&rsp[8], CMDLEN_MAXBYTES / 4 - 2, &app_version, 1);

		app_reply(*hdr, RSP_NAME_VERSION, rsp);
		// still initial state
		break;

	case CMD_LOAD_APP: {
		uint32_t local_app_size;

		qemu_puts("cmd: load-app(size, uss)\n");
		if (hdr->len != 128) {
			// Bad length
			state = STATE_FAIL;
			break;
		}

		// cmd[1..4] contains the size.
		local_app_size =
		    cmd[1] + (cmd[2] << 8) + (cmd[3] << 16) + (cmd[4] << 24);

		qemu_puts("app size: ");
		qemu_putinthex(local_app_size);
		qemu_lf();

		if (local_app_size == 0 || local_app_size > TK1_APP_MAX_SIZE) {
			rsp[0] = STATUS_BAD;
			app_reply(*hdr, RSP_LOAD_APP, rsp);
			// still initial state
			break;
		}

		ctx->left = local_app_size;

		// Do we have a USS at all?
		if (cmd[5] != 0) {
			// Yes
			ctx->use_uss = true;
			memcpy_s(ctx->uss, 32, &cmd[6], 32);
		}

		rsp[0] = STATUS_OK;
		app_reply(*hdr, RSP_LOAD_APP, rsp);

		state = STATE_LOADING;
		break;
	}
	case CMD_UNREGISTER_MGMT_APP: {
		qemu_puts("cmd: unregister mgmt app\n");
		if (hdr->len != 1) {
			// Bad length
			state = STATE_FAIL;
			break;
		}

		int ret = unregister_mgmt();

		if (ret == 0) {
			rsp[0] = STATUS_OK;
			app_reply(*hdr, RSP_UNREGISTER_MGMT_APP, rsp);
			// still initial state
			break;
		}
		rsp[0] = STATUS_BAD;
		app_reply(*hdr, RSP_UNREGISTER_MGMT_APP, rsp);
		// still initial state
		break;
	}

	case CMD_REGISTER_MGMT_APP: {
		qemu_puts("cmd: register mgmt app\n");
		if (hdr->len != 1) {
			// Bad length
			state = STATE_FAIL;
			break;
		}

		int ret = register_mgmt();

		if (ret == 0) {
			rsp[0] = STATUS_OK;
			app_reply(*hdr, RSP_REGISTER_MGMT_APP, rsp);
			// still initial state
			break;
		}
		rsp[0] = STATUS_BAD;
		app_reply(*hdr, RSP_REGISTER_MGMT_APP, rsp);
		// still initial state
		break;
	}

	case CMD_DELETE_APP: {
		qemu_puts("cmd: delete app\n");
		if (hdr->len != 1) {
			// Bad length
			state = STATE_FAIL;
			break;
		}

		int ret = preload_delete();

		if (ret == 0) {
			rsp[0] = STATUS_OK;
			app_reply(*hdr, RSP_DELETE_APP, rsp);
			// still initial state
			break;
		}
		rsp[0] = STATUS_BAD;
		app_reply(*hdr, RSP_DELETE_APP, rsp);
		// still initial state
		break;
	}

	default:
		qemu_puts("Got unknown cmd: 0x");
		qemu_puthex(cmd[0]);
		qemu_lf();
		state = STATE_FAIL;
		break;
	}

	return state;
}

static enum state loading_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};
	uint32_t nbytes = 0;

	switch (cmd[0]) {
	case CMD_LOAD_APP_DATA:
		qemu_puts("cmd: load-app-data\n");
		if (hdr->len != 128) {
			// Bad length
			state = STATE_FAIL;
			break;
		}

		if (ctx->left > (128 - 1)) {
			nbytes = 128 - 1;
		} else {
			nbytes = ctx->left;
		}
		memcpy_s(ctx->loadaddr + ctx->count, ctx->left, cmd + 1,
			 nbytes);

		ctx->left -= nbytes;
		ctx->count += nbytes;

		if (ctx->count >= 4096) {

			// write one 4K page to flash
			preload_store(ctx, 4096);

			// rearrange the buffer
			int left_in_buf = ctx->count - 4096;
			uint8_t *src_addr =
			    ctx->loadaddr + ctx->count - left_in_buf;

			memcpy_s(ctx->loadaddr, 4096 + 256, src_addr,
				 left_in_buf);

			ctx->count = left_in_buf;
		}

		if (ctx->left == 0) {
			qemu_puts("Fully loaded ");
			qemu_lf();

			// Do the last store, if something is left
			if (ctx->count != 0) {
				preload_store(ctx, ctx->count);
			}



			// TODO: implement digest check, and send to client
			/*int digest_err = compute_app_digest(ctx->digest);*/
			/*assert(digest_err == 0);*/
			/*print_digest(ctx->digest);*/

			// And return the digest in final
			// response
			rsp[0] = STATUS_OK;
			/*memcpy_s(&rsp[1], CMDLEN_MAXBYTES - 1, ctx->digest,*/
			/*	 32);*/
			app_reply(*hdr, RSP_LOAD_APP_DATA_READY, rsp);

			state = STATE_RUN;
			break;
		}

		rsp[0] = STATUS_OK;
		app_reply(*hdr, RSP_LOAD_APP_DATA, rsp);
		// still loading state
		break;

	default:
		qemu_puts("Got unknown firmware cmd: 0x");
		qemu_puthex(cmd[0]);
		qemu_lf();
		state = STATE_FAIL;
		break;
	}

	return state;
}

int main(void)
{
	struct context ctx = {0};
	struct frame_header hdr = {0};
	uint8_t cmd[CMDLEN_MAXBYTES] = {0};
	enum state state = STATE_INITIAL;
	uint8_t app_buf[4096 + 256] = {
	    0x00}; // 4K page + room for an extra frame

	ctx.loadaddr = app_buf;
	ctx.count = 0;
	ctx.use_uss = false;

	for (;;) {
		switch (state) {
		case STATE_INITIAL:
			if (read_command(&hdr, cmd) == -1) {
				state = STATE_FAIL;
				break;
			}

			state = initial_commands(&hdr, cmd, state, &ctx);
			break;

		case STATE_LOADING:
			if (read_command(&hdr, cmd) == -1) {
				state = STATE_FAIL;
				break;
			}

			state = loading_commands(&hdr, cmd, state, &ctx);

		case STATE_RUN:
			// Force a reset to authenticate app
			break;

		case STATE_FAIL:
			// fallthrough
		default:
			qemu_puts("firmware state 0x");
			qemu_puthex(state);
			qemu_lf();
			assert(1 == 2);
			break; // Not reached
		}
	}

	/* We don't care about memory leaks here. */
	return (int)0xcafebabe;
}
