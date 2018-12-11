#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "jsmn/jsmnutil.h"
#include "utils.h"
#include "debug.h"
#include "rlpc.h"

static int _rlp_append(uint8_t **rlp, int *idx, int *rlen, uint8_t *a, int len, uint8_t ***rlp_out)
{
	int write = len;

	if (*rlen <= *idx) {
		int old = *rlen;
		*rlen = old + 1;
		rlp = k_realloc(rlp, *rlen * sizeof(uint8_t *), old * sizeof(uint8_t *));
	}

	if (!len)
		goto out;

	// append at index
	rlp[*idx] = k_calloc(1, len * sizeof(uint8_t));
	len--;
	do {
		rlp[*idx][len] = a[len];
	} while (len--);

out:
	*rlp_out = rlp;

	return write;
}

static int _rlp_decode(uint8_t ***rlp, int *idx, int *rlen, uint8_t *d, int *size, int **dsize, int *off)
{
	int ret = 0;
	unsigned short len = 0;

	if (*dsize == 0)
		*dsize = k_malloc(128 * sizeof(int));

	if (!d)
		goto out;

	if (d[0] < 0x80) {
		*off += _rlp_append(*rlp, idx, rlen, &d[0], 1, rlp);
		(*dsize)[*idx] = *off;
		*idx += 1;
	} else if(d[0] < 0xb8) {
		len = d[0] - 0x80;
		*off += _rlp_append(*rlp, idx, rlen, d+1, len, rlp);
		(*dsize)[*idx] = *off;
		*idx += 1;
		*off += 1;
	} else if(d[0] < 0xc0) {
		int llen = d[0] - 0xb7;
		for (int i = 1; i <= llen; i++)
			len |= ((char) d[i] << (8*(llen-i)));
		*off += _rlp_append(*rlp, idx, rlen, d+1+llen, len, rlp);
		(*dsize)[*idx] = *off;
		*idx += 1;
		*off += llen+1;
	} else {
		if (d[0] == 0) {
			ret = 0;
		} else if (d[0] < 0xf8) {
		} else if (d[0] < 0x100) {
			len = 0;
			int llen = d[0] - 0xf7;
			for (int i = 1; i <= llen; i++)
				len |= ((char) d[i] << (8*(llen-i)));
			int at = llen+1;
			int tries = 0;
			len += at;
			do {
				int offset = 0;
				ret = _rlp_decode(rlp, idx, rlen, &d[at], size, dsize, &offset);
				at += offset;
				tries++;
			} while (at < len);
		} else {
		}
	}

out:
	*size = (*idx);
	return ret;
}

static void _rlp_k_free(uint8_t **rlp, int size, int *dsize)
{
	for (int j = 0; j < size; j++) {
		if (dsize[j])
			k_free(rlp[j]);
	}

	k_free(dsize);
	k_free(rlp);
}

static void _rlp_print(uint8_t **rlp, int size, int *dsize)
{
	dbg_log_raw("\nData (size=%d): [", size);
	for (int j = 0; j < size; j++) {
		for (int i = 0; i < dsize[j]; i++) {
			if (i == 0)
				dbg_log_raw("\n  [(%d): '", dsize[j]);
			dbg_log_raw("%02x", rlp[j][i]);
			if (i == (dsize[j]-1))
				dbg_log_raw("']");
		}
		if (dsize[j] == 0)
			dbg_log_raw("\n  [(%d): '']", 0);
		if (j != (size-1))
			dbg_log_raw(",");

	}
	dbg_log_raw("\n]\n\n");
}

rlp_ctx_t *rlp_start_str(char *str)
{
	bytes_t *b = k_calloc(1, sizeof(bytes_t));

	b->len = str2byte_a(str, &b->data);

	return rlp_start_buf(b);
}

rlp_ctx_t *rlp_start_buf(bytes_t *buf)
{
	rlp_ctx_t *ctx = k_calloc(1, sizeof(rlp_ctx_t));

	_rlp_decode(&ctx->rlp, &ctx->idx, &ctx->rlen, buf->data, &ctx->size,
		&ctx->dsize, &ctx->off);

	b_free(buf);

	return ctx;
}

int rlp_size(rlp_ctx_t *ctx)
{
	if (ctx)
		return ctx->size;

	return 0;
}

void rlp_print_one(rlp_ctx_t *ctx, bytes_t *b)
{
	dbg_log_raw("Buffer: ");
	for (int i = 0; i < b->len; i++)
		dbg_log_raw("%02x", b->data[i]);
	dbg_log_raw("\n");
}

bytes_t *rlp_get_buf(rlp_ctx_t *ctx, int at)
{
	if (!ctx)
		return NULL;

	if (at < 0)
		return NULL;

	bytes_t *b = k_calloc(1, sizeof(bytes_t));

	b->len = ctx->dsize[at];
	b->data = k_calloc(1, b->len);
	b->data = memcpy(b->data, ctx->rlp[at], b->len);

	return b;
}

void rlp_print(rlp_ctx_t *ctx)
{
	if (ctx)
		_rlp_print(ctx->rlp, ctx->size, ctx->dsize);
}

void rlp_finish(rlp_ctx_t *ctx)
{
	if (ctx)
		_rlp_k_free(ctx->rlp, ctx->size, ctx->dsize);

	k_free(ctx);
}
