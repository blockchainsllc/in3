#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//include <zephyr.h>

#include "bytes.h"

bytes_t *b_new(char *data, int len)
{
	bytes_t *b = calloc(1, sizeof(bytes_t));

	b->len = len;
	b->data = calloc(1, len);
	b->data = memcpy(b->data, data, len);

	return b;
}

void b_print(bytes_t *a)
{
	int i;
	if (!a)
		return;

	printf("Bytes: ");
	for (i = 0; i < a->len; i++)
		printf("%02x", a->data[i]);
	printf("\n");
}

int b_cmp(bytes_t *a, bytes_t *b)
{
	int i = 0;

	if ((a && b) == 0)
		return 1;

	for (i = 0; i < a->len; i++) {
		if (a->data[i] != b->data[i])
			break;
	}

	return (i != a->len);
}

void b_free(bytes_t *a)
{
	if (!a)
		return;

	free(a->data);
	free(a);
}

bytes_t *b_dup(bytes_t *a)
{
	bytes_t *out = calloc(1, sizeof(bytes_t));

	out->data = calloc(1, a->len);
	out->data = memcpy(out->data, a->data, a->len);
	out->len = a->len;

	return out;
}
