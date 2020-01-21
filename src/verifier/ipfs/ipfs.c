
#include "../../core/util/mem.h"
#include "ipfs.pb.h"
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdio.h>
#include <stdlib.h>

#define GOTO_RET(label, val) \
  do {                       \
    ret = val;               \
    goto label;              \
  } while (0)

typedef struct data_ {
  size_t         len;
  const uint8_t* buf;
} cb_arg_bytes_t;

static bool cb_encode_bytes(pb_ostream_t* stream, const pb_field_t* field, void* const* arg) {
  cb_arg_bytes_t* data = *arg;
  return pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, data->buf, data->len);
}

static size_t pb_encode_size(const pb_msgdesc_t* fields, const void* src_struct) {
  pb_ostream_t s_ = PB_OSTREAM_SIZING;
  if (pb_encode(&s_, fields, src_struct))
    return s_.bytes_written;
  return 0;
}

int ipfs_create_hash(const uint8_t* content, size_t len, int hash) {
  int            ret = 0;
  cb_arg_bytes_t tmp = {.buf = NULL, .len = 0};
  pb_ostream_t   stream;
  size_t         wlen = 0;
  uint8_t *      buf1 = NULL, *buf2 = NULL;

  Data data              = Data_init_zero;
  data.Type              = Data_DataType_File;
  data.has_filesize      = true;
  data.filesize          = len;
  data.Data.funcs.encode = &cb_encode_bytes;
  tmp.buf                = content;
  tmp.len                = len;
  data.Data.arg          = &tmp;

  wlen = pb_encode_size(Data_fields, &data);
  if ((buf1 = _malloc(wlen)) == NULL)
    GOTO_RET(EXIT, -2);

  stream = pb_ostream_from_buffer(buf1, wlen);
  if (!pb_encode(&stream, Data_fields, &data))
    GOTO_RET(EXIT, -1);

  PBNode node            = PBNode_init_zero;
  node.Data.funcs.encode = &cb_encode_bytes;
  tmp.buf                = buf1;
  tmp.len                = stream.bytes_written;
  node.Data.arg          = &tmp;

  wlen = pb_encode_size(PBNode_fields, &node);
  if ((buf2 = _malloc(wlen)) == NULL)
    GOTO_RET(EXIT, -2);

  stream = pb_ostream_from_buffer(buf2, wlen);
  if (!pb_encode(&stream, PBNode_fields, &node))
    GOTO_RET(EXIT, -1);

EXIT:
  _free(buf2);
  _free(buf1);
  return ret;
}

int main() {
  const uint8_t buf[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4};
  ipfs_create_hash(buf, 10, 0);
  return 0;
}
