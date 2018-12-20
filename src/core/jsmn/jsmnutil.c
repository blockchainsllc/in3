#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "jsmnutil.h"
#include "jsmn.h"

typedef void (*token_iter_f) (void *data, const char *buf, jsmntok_t* tok, int c);

static int
traverse_token (const char *buf, jsmntok_t* tok)
{
	int i;
	int c=1;
	for (i=0; i < tok->size; i++) {
		c += traverse_token (buf, tok+c);
	}
	return c;
}

// is good for getting elements of any array type token. Just point tok+t to the
// token of type array and it will iterate the direct children of that token
// through travesal the depth first token array.
static int
iterate_json_item(const char *buf, jsmntok_t* tok, token_iter_f func, void *data)
{
	int i;
	int c=0;

	if (tok[0].type != JSMN_ARRAY && tok[0].type != JSMN_OBJECT)
		return JSMNUTIL_ERROR_TOKTYPE;

	c++;
	for(i=0; i < tok->size; i++) {
		func(data, buf, tok+c, i);
		c+=traverse_token (buf, tok+c);
	}

	return JSMNUTIL_OK;
}

static void
tok_arr_append_cb(void *data, const char* buf, jsmntok_t *tok, int c)
{
	jsmntok_t **arr = (jsmntok_t **) data;
	// XXX: figure get_array_toks without the +1 offset.
	arr[c] = tok;
}

int
jsmnutil_parse_json (const char *buf, jsmntok_t **jsonv_out, int *jsons_out)
{
	return jsmnutil_parse_json_range(buf, strlen(buf), jsonv_out, jsons_out);
}

int
jsmnutil_parse_json_range (const char *buf, size_t len,jsmntok_t **jsonv_out, int *jsons_out)
{
	jsmn_parser parser;
	int r, offset;

	jsmn_init (&parser);

	*jsons_out=10;
	*jsonv_out = _calloc(1, *jsons_out * sizeof(jsmntok_t));

	if (*jsonv_out == NULL) {
		fprintf(stderr, "k_malloc(): errno=%d\n", errno);
		return 0;
	}
again:
	r = jsmn_parse(&parser, buf, len, *jsonv_out,
		       *jsons_out);

	if (r < 0) {
		if (r == JSMN_ERROR_NOMEM) {
			offset = *jsons_out;
			int size = *jsons_out;
			*jsons_out = *jsons_out * 2;
			*jsonv_out = _realloc(*jsonv_out, sizeof(jsmntok_t)
					     * *jsons_out, size * sizeof(jsmntok_t));
			memset(*jsonv_out+offset, 0, sizeof(jsmntok_t) * offset);
			if (jsonv_out == NULL) {
				return 0;
			}
			goto again;
		}
	}
	return r;
}


int
jsmnutil_array_count(const char *buf, jsmntok_t* tok)
{
	if (tok[0].type != JSMN_ARRAY) {
		printf ("provided tok is not of type JSNM_ARRAY\n");
		return -1;
	}

	return tok[0].size;
}

jsmntok_t**
jsmnutil_get_array_toks (const char *buf, jsmntok_t *tok)
{
	jsmntok_t** arr;

	if(tok->type != JSMN_ARRAY) {
		printf ("ERROR: jsmnutil_get_array_toks expects JSMN_ARRAY(%d) tok as input; found: %d\n",
			JSMN_ARRAY, tok->type);
		return NULL;
	}

	arr = _malloc(sizeof(jsmntok_t*) * (tok->size + 1));
	arr[tok->size] = NULL; // NULL terminated

	iterate_json_item(buf, tok, tok_arr_append_cb, arr);

	return arr;
}

int
jsmnutil_object_key_count(const char *buf, jsmntok_t* tok)
{
	if (tok[0].type != JSMN_OBJECT) {
		printf ("provided tok is not of type JSNM_ARRAY\n");
		return -1;
	}

	return tok->size;
}

jsmntok_t**
jsmnutil_get_object_keys (const char *buf, jsmntok_t *tok)
{
	jsmntok_t** arr;
	int c;

	if(tok->type != JSMN_OBJECT) {
		printf ("ERROR: jsmnutil_get_array_toks expects JSMN_OBJECT(%d) tok as input; found: %d\n",
			JSMN_ARRAY, tok->type);
		return NULL;
	}

	c = jsmnutil_object_key_count(buf, tok);
	arr = _malloc(sizeof(jsmntok_t*) * (c + 1));
	arr[c] = NULL; // NULL terminated

	iterate_json_item(buf, tok, tok_arr_append_cb, arr);

	return arr;
}

jsmntok_t*
jsmnutil_get_object_key_value (const char *buf, jsmntok_t *tok)
{
	// values are always right next to the key...
	return tok+1;
}

void
jsmnutil_tokv_k_free(jsmntok_t** tok)
{
	if (!tok)
		return;

	// do not k_free tokens as they are managed by
	// token array memory blocked created during
	// parsing
	_free (tok);
}
