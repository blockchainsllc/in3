
#ifndef IN3_PARAMS_H
#define IN3_PARAMS_H

#include "data.h"
#include "stringbuilder.h"

// @PUBLIC_HEADER
// macros for checking params

/* checks that the params have at least the given len */
#define CHECK_PARAMS_LEN(ctx, params, len) \
  if (d_type(params) != T_ARRAY || d_len(params) < len) return rpc_throw(ctx, "arguments need to be a array with at least %u arguments", len);

/* checks that the parameter at the given index is of the specified type */
#define CHECK_PARAM_TYPE(ctx, params, index, type)                                                                    \
  {                                                                                                                   \
    if (type == T_BYTES) d_bytes(d_get_at(params, index));                                                            \
    if (d_type(d_get_at(params, index)) != type) return rpc_throw(ctx, "argument %u must be a %s", index + 1, #type); \
  }

/* checks that the parameter at the given index is a number */
#define CHECK_PARAM_NUMBER(ctx, params, index)                                 \
  d_num_bytes(d_get_at(params, index));                                        \
  switch (d_type(d_get_at(params, index))) {                                   \
    case T_INTEGER:                                                            \
    case T_BYTES: break;                                                       \
    default: return rpc_throw(ctx, "argument %u must be a number", index + 1); \
  }

/* checks that the parameter at the given index is a address */
#define CHECK_PARAM_ADDRESS(ctx, params, index)                                                                                    \
  {                                                                                                                                \
    d_token_t* val = d_get_at(params, index);                                                                                      \
    if (d_bytes(val).len != 20 || d_type(val) != T_BYTES) return rpc_throw(ctx, "argument %u must be a valid address", index + 1); \
  }

/* checks that the parameter at the given index has exactly the given len*/
#define CHECK_PARAM_LEN(ctx, params, index, len) \
  if (d_len(d_get_at(params, index)) != len) return rpc_throw(ctx, "argument %u must have a length of %u", index + 1, len);

/* checks that the parameter at the given index has a len greater than the given amount*/
#define CHECK_PARAM_MAX_LEN(ctx, params, index, len) \
  if (d_len(d_get_at(params, index)) > len) return rpc_throw(ctx, "argument %u must be smaller than %u", index + 1, len);

/* checks that the parameter at the given index has a len less than the given amount*/
#define CHECK_PARAM_MIN_LEN(ctx, params, index, len) \
  if (d_len(d_get_at(params, index)) < len) return rpc_throw(ctx, "argument %u must be at least %u", index + 1, len);

/* checks that the parameter at the given index fulfills the given condition*/
#define CHECK_PARAM(ctx, params, index, cond)                                          \
  {                                                                                    \
    d_token_t* val = d_get_at(params, index);                                          \
    if (!(cond)) return rpc_throw(ctx, "argument %u must match %s", index + 1, #cond); \
  }

/* fetches the parameter with the given index as bytes and stores it in the target, which must be a bytes_t variable*/
#define TRY_PARAM_GET_BYTES(target, ctx, index, min_len, max_len)                                                                            \
  {                                                                                                                                          \
    d_token_t* t = d_get_at(ctx->params, index);                                                                                             \
    if (d_type(t) == T_NULL)                                                                                                                 \
      target = NULL_BYTES;                                                                                                                   \
    else if (!d_is_bytes(t) && d_type(t) != T_INTEGER)                                                                                       \
      return rpc_throw(ctx->req, "Argument %u must be bytes!", index + 1);                                                                   \
    else {                                                                                                                                   \
      target = d_bytes(t);                                                                                                                   \
      if ((int) target.len < (int) min_len) return rpc_throw(ctx->req, "Argument %u must have at least a length of %u", index + 1, min_len); \
      if (max_len && (int) target.len > (int) max_len) return rpc_throw(ctx->req, "Argument %u must have max %u bytes", index + 1, max_len); \
    }                                                                                                                                        \
  }

/* fetches the parameter with the given index as bytes and stores it in the target, which must be a bytes_t variable*/
#define TRY_PARAM_GET_UINT256(target, ctx, index)                                                            \
  {                                                                                                          \
    d_token_t* t = d_get_at(ctx->params, index);                                                             \
    if (d_type(t) == T_NULL)                                                                                 \
      target = NULL_BYTES;                                                                                   \
    else if (!d_is_bytes(t) && d_type(t) != T_INTEGER && !d_num_bytes(t).data)                               \
      return rpc_throw(ctx->req, "Argument %u must be a numeric value!", index + 1);                         \
    else {                                                                                                   \
      target = d_num_bytes(t);                                                                               \
      if (target.len > 32) return rpc_throw(ctx->req, "Argument %u must have max 32 bytes long", index + 1); \
    }                                                                                                        \
  }

#define TRY_PARAM_GET_REQUIRED_UINT256(target, ctx, index)                                                   \
  {                                                                                                          \
    d_token_t* t = d_get_at(ctx->params, index);                                                             \
    if (d_type(t) == T_NULL || (!d_is_bytes(t) && d_type(t) != T_INTEGER && !d_num_bytes(t).data))           \
      return rpc_throw(ctx->req, "Argument %u must be a numeric value!", index + 1);                         \
    else {                                                                                                   \
      target = d_num_bytes(t);                                                                               \
      if (target.len > 32) return rpc_throw(ctx->req, "Argument %u must have max 32 bytes long", index + 1); \
    }                                                                                                        \
  }

/* fetches the required parameter with the given index as bytes and stores it in the target, which must be a bytes_t variable*/
#define TRY_PARAM_GET_REQUIRED_BYTES(target, ctx, index, min_len, max_len)                                                                 \
  {                                                                                                                                        \
    target = d_bytes(d_get_at(ctx->params, index));                                                                                        \
    if (!target.data) return rpc_throw(ctx->req, "Argument %u must be bytes!", index + 1);                                                 \
    if ((int) target.len < (int) min_len) return rpc_throw(ctx->req, "Argument %u must have at least a length of %u", index + 1, min_len); \
    if (max_len && (int) target.len > (int) max_len) return rpc_throw(ctx->req, "Argument %u must have max %u bytes", index + 1, max_len); \
  }

/* fetches the parameter with the given index as bytes and stores it in the target, which must be a bytes_t variable*/
#define TRY_PARAM_GET_BYTES_AS(target, ctx, index, min_len, max_len, enc)                                                                    \
  {                                                                                                                                          \
    d_token_t* t = d_get_at(ctx->params, index);                                                                                             \
    if (d_type(t) == T_NULL)                                                                                                                 \
      target = NULL_BYTES;                                                                                                                   \
    else {                                                                                                                                   \
      target = d_bytes_enc(t, enc);                                                                                                          \
      if (!target.data) return rpc_throw(ctx->req, "Argument %u must be encoded as " #enc, index + 1);                                       \
      if ((int) target.len < (int) min_len) return rpc_throw(ctx->req, "Argument %u must have at least a length of %u", index + 1, min_len); \
      if (max_len && (int) target.len > (int) max_len) return rpc_throw(ctx->req, "Argument %u must have max %u bytes", index + 1, max_len); \
    }                                                                                                                                        \
  }

/* fetches the required parameter with the given index as bytes and stores it in the target, which must be a bytes_t variable*/
#define TRY_PARAM_GET_REQUIRED_BYTES_AS(target, ctx, index, min_len, max_len, enc)                                                         \
  {                                                                                                                                        \
    target = d_bytes_enc(d_get_at(ctx->params, index), enc);                                                                               \
    if (!target.data) return rpc_throw(ctx->req, "Argument %u must be valid bytes as " #enc, index + 1);                                   \
    if ((int) target.len < (int) min_len) return rpc_throw(ctx->req, "Argument %u must have at least a length of %u", index + 1, min_len); \
    if (max_len && (int) target.len > (int) max_len) return rpc_throw(ctx->req, "Argument %u must have max %u bytes", index + 1, max_len); \
  }

/* fetches the parameter with the given index as integer and stores it in the target, which must be a int or int32_t variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_INT(target, ctx, index, def)                                    \
  {                                                                                   \
    d_token_t* t = d_get_at(ctx->params, index);                                      \
    d_num_bytes(t);                                                                   \
    if (d_type(t) == T_NULL)                                                          \
      target = def;                                                                   \
    else if (d_type(t) != T_INTEGER)                                                  \
      return rpc_throw(ctx->req, "Argument %u must be an integer value!", index + 1); \
    else                                                                              \
      target = d_int(t);                                                              \
  }

/* fetches the required parameter with the given index as integer and stores it in the target, which must be a int or int32_t variable*/
#define TRY_PARAM_GET_REQUIRED_INT(target, ctx, index)                                \
  {                                                                                   \
    d_token_t* t = d_get_at(ctx->params, index);                                      \
    d_num_bytes(t);                                                                   \
    if (d_type(t) != T_INTEGER)                                                       \
      return rpc_throw(ctx->req, "Argument %u must be an integer value!", index + 1); \
    else                                                                              \
      target = d_int(t);                                                              \
  }

/* fetches the required parameter with the given index as integer and stores it in the target, which must be a int or int32_t variable*/
#define TRY_PARAM_GET_REQUIRED_BOOL(target, ctx, index)                               \
  {                                                                                   \
    d_token_t* t = d_get_at(ctx->params, index);                                      \
    if (d_type(t) != T_BOOLEAN)                                                       \
      return rpc_throw(ctx->req, "Argument %u must be an boolean value!", index + 1); \
    else                                                                              \
      target = d_int(t);                                                              \
  }

/* fetches the parameter with the given index as unsigned long integer and stores it in the target, which must be a uint64_t variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_LONG(target, ctx, index, def)                                \
  {                                                                                \
    d_token_t* t = d_get_at(ctx->params, index);                                   \
    d_num_bytes(t);                                                                \
    if (d_type(t) == T_NULL)                                                       \
      target = def;                                                                \
    else if (d_type(t) == T_INTEGER || (d_type(t) == T_BYTES && d_len(t) <= 8))    \
      target = d_long(t);                                                          \
    else                                                                           \
      return rpc_throw(ctx->req, "Argument %u must be an long value!", index + 1); \
  }

/* fetches the required parameter with the given index as unsigned long integer and stores it in the target, which must be a uint64_t variable*/
#define TRY_PARAM_GET_REQUIRED_LONG(target, ctx, index)                            \
  {                                                                                \
    d_token_t* t = d_get_at(ctx->params, index);                                   \
    d_num_bytes(t);                                                                \
    if (d_type(t) == T_INTEGER || (d_type(t) == T_BYTES && d_len(t) <= 8))         \
      target = d_long(t);                                                          \
    else                                                                           \
      return rpc_throw(ctx->req, "Argument %u must be an long value!", index + 1); \
  }

/* fetches the parameter with the given index as boolean and stores it in the target, which must be a bool or int variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_BOOL(target, ctx, index, def)                                   \
  {                                                                                   \
    d_token_t* t = d_get_at(ctx->params, index);                                      \
    if (d_type(t) == T_NULL)                                                          \
      target = def;                                                                   \
    else if (d_type(t) != T_BOOLEAN)                                                  \
      return rpc_throw(ctx->req, "Argument %u must be an true or false!", index + 1); \
    else                                                                              \
      target = d_int(t);                                                              \
  }

/* fetches the parameter with the given index as address (20 bytes) and stores it in the target, which must be a uint8_t* variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_ADDRESS(target, ctx, index, def)                               \
  {                                                                                  \
    d_token_t* t = d_get_at(ctx->params, index);                                     \
    if (d_type(t) == T_NULL)                                                         \
      target = def;                                                                  \
    else if (!d_is_bytes(t) || d_bytes(t).len != 20)                                 \
      return rpc_throw(ctx->req, "Argument %u must be a valid address!", index + 1); \
    else                                                                             \
      target = d_bytes(t).data;                                                      \
  }

/* fetches the required parameter with the given index as address (20 bytes) and stores it in the target, which must be a uint8_t* variable*/
#define TRY_PARAM_GET_REQUIRED_ADDRESS(target, ctx, index)                           \
  {                                                                                  \
    d_token_t* t = d_get_at(ctx->params, index);                                     \
    if (!d_is_bytes(t) || d_bytes(t).len != 20)                                      \
      return rpc_throw(ctx->req, "Argument %u must be a valid address!", index + 1); \
    else                                                                             \
      target = d_bytes(t).data;                                                      \
  }

/* fetches the parameter with the given index as string and stores it in the target, which must be a char* variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_STRING(target, ctx, index, def)                                 \
  {                                                                                   \
    d_token_t* t = d_get_at(ctx->params, index);                                      \
    switch (d_type(t)) {                                                              \
      case T_NULL:                                                                    \
        target = def;                                                                 \
        break;                                                                        \
      case T_BYTES:                                                                   \
        target = alloca(d_len(t) * 2 + 3);                                            \
        bytes_to_hex_string(target, "0x", d_bytes(t), NULL);                          \
        break;                                                                        \
      case T_INTEGER:                                                                 \
        target = alloca(10);                                                          \
        sprintf(target, "%d", d_int(t));                                              \
        break;                                                                        \
      case T_STRING:                                                                  \
        target = d_string(t);                                                         \
        break;                                                                        \
      default:                                                                        \
        return rpc_throw(ctx->req, "Argument %u must be a valid string!", index + 1); \
    }                                                                                 \
  }

/* fetches the required parameter with the given index as string and stores it in the target, which must be a char* variable.*/
#define TRY_PARAM_GET_REQUIRED_STRING(target, ctx, index)                                      \
  {                                                                                            \
    TRY_PARAM_GET_STRING(target, ctx, index, NULL)                                             \
    if (!target) return rpc_throw(ctx->req, "Argument %u must be a valid string!", index + 1); \
  }

/* fetches the parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json object, but allows NULL, if it is not given.*/
#define TRY_PARAM_GET_OBJECT(target, ctx, index)                                    \
  {                                                                                 \
    target = d_get_at(ctx->params, index);                                          \
    if (d_type(target) == T_NULL)                                                   \
      target = NULL;                                                                \
    else if (d_type(target) != T_OBJECT)                                            \
      return rpc_throw(ctx->req, "Argument %u must be a valid object!", index + 1); \
  }

/* fetches the required parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json object.*/
#define TRY_PARAM_GET_REQUIRED_OBJECT(target, ctx, index)                           \
  {                                                                                 \
    target = d_get_at(ctx->params, index);                                          \
    if (d_type(target) != T_OBJECT)                                                 \
      return rpc_throw(ctx->req, "Argument %u must be a valid object!", index + 1); \
  }

/* fetches the parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json array, but allows NULL, if it is not given.*/
#define TRY_PARAM_GET_ARRAY(target, ctx, index)                                    \
  {                                                                                \
    target = d_get_at(ctx->params, index);                                         \
    if (d_type(target) == T_NULL)                                                  \
      target = NULL;                                                               \
    else if (d_type(target) != T_ARRAY)                                            \
      return rpc_throw(ctx->req, "Argument %u must be a valid array!", index + 1); \
  }

/* fetches the required parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json array.*/
#define TRY_PARAM_GET_REQUIRED_ARRAY(target, ctx, index)                           \
  {                                                                                \
    target = d_get_at(ctx->params, index);                                         \
    if (d_type(target) != T_ARRAY)                                                 \
      return rpc_throw(ctx->req, "Argument %u must be a valid array!", index + 1); \
  }

#define TRY_PARAM_CONVERT_REQUIRED_OBJECT(dst, ctx, index, fn) \
  {                                                            \
    d_token_t* ob;                                             \
    TRY_PARAM_GET_REQUIRED_OBJECT(ob, ctx, index)              \
    TRY(fn(ctx->req, ob, &dst))                                \
  }

#define TRY_PARAM_CONVERT_OBJECT(dst, ctx, index, fn) \
  {                                                   \
    d_token_t* ob;                                    \
    TRY_PARAM_GET_OBJECT(ob, ctx, index)              \
    if (ob) TRY(fn(ctx->req, ob, &dst))               \
  }

#endif