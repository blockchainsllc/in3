#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>  
//#include <zephyr.h>

//#include "fsm.h"
#include "merkle.h"
#include "../jsmn/jsmnutil.h"
#include "utils.h"
#include "rlpc.h"

#include "debug.h"

// converts the byte array to nibles of 4 bit each
static int8_t *str_to_nibbles(bytes_t *path)
	int8_t *n = calloc(1, 1 + (path->len * 2));

	for (int i = 0; i < path->len; i++) {
		n[i] = (path->data[i] >> 4);
		if ((i+1) <= path->len)
			n[i+1] = (path->data[i] & 0xF);
	}

    // TODO Do we need this?
	n[path->len * 2] = -1;

	return n;
}

int verifyMerkleProof(bytes_t *rhash, bytes_t *path, bytes_t **proof, bytes_t *needle)
{
	int i = 0, ret = 1;
	rlp_ctx_t *ctx = 0;
	bytes_t *p, *sha, *want, *find = 0;

	int8_t *n = str_to_nibbles(path);
	want = rhash;

	while (*proof) {
		p = *proof;
		sha = sha3(p);
		if (b_cmp(sha, want)) {
			printk("Wrong rootHash\n");
			goto out;
		}
		b_free(sha);
		sha = 0;
		b_free(want);
		want = 0;
		if (ctx) {
			rlp_finish(ctx);
			ctx = 0;
		}
		ctx = rlp_start_buf(p);
		rlp_print(ctx);

		if ((rlp_size(ctx) == 2) || (n[i] < 0)) {
			find = rlp_get_buf(ctx, 1);
			if (b_cmp(find, needle)) {
				goto out;
			}
		} else {
			bytes_t *t = rlp_get_buf(ctx, n[i]);
			want = t;
		}

		proof++;
		i++;
	}

	ret = 0;

out:
	if (n)
		free(n);
	if (ctx)
		rlp_finish(ctx);
	if (sha)
		b_free(sha);
	if (want)
		b_free(want);
	if (find)
		b_free(find);

	return ret;
}
