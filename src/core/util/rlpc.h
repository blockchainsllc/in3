#include "bytes.h"

#ifndef RLPC_H
#define RLPC_H

/* RLP Contextb */
typedef struct {
	int size;
	int rlen;
	int idx;
	int off;
	int *dsize;
	uint8_t **rlp;
} rlp_ctx_t;

rlp_ctx_t *rlp_start_str(char *str);
rlp_ctx_t *rlp_start_buf(bytes_t *buf);
bytes_t *rlp_get_buf(rlp_ctx_t *ctx, int at);
int rlp_size(rlp_ctx_t *ctx);
void rlp_print_one(rlp_ctx_t *ctx, bytes_t *b);
void rlp_print(rlp_ctx_t *ctx);
void rlp_finish(rlp_ctx_t *ctx);

#endif
