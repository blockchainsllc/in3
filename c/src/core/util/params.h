
#ifndef IN3_PARAMS_H
#define IN3_PARAMS_H

#include "data.h"
#include "stringbuilder.h"

// @PUBLIC_HEADER
// macros for checking params

/* checks that the params have at least the given len */
#define CHECK_PARAMS_LEN(ctx, params, len) \
  if (d_type(params) != T_ARRAY || d_len(params) < len) return req_set_error(ctx, "arguments need to be a array with at least " #len " arguments", IN3_EINVAL);

/* checks that the parameter at the given index is of the specified type */
#define CHECK_PARAM_TYPE(ctx, params, index, type) \
  if (d_type(d_get_at(params, index)) != type) return req_set_error(ctx, "argument at index " #index " must be a " #type, IN3_EINVAL);

/* checks that the parameter at the given index is a number */
#define CHECK_PARAM_NUMBER(ctx, params, index)                                                       \
  switch (d_type(d_get_at(params, index))) {                                                         \
    case T_INTEGER:                                                                                  \
    case T_BYTES: break;                                                                             \
    default: return req_set_error(ctx, "argument at index " #index " must be a number", IN3_EINVAL); \
  }

/* checks that the parameter at the given index is a address */
#define CHECK_PARAM_ADDRESS(ctx, params, index)                                                                                                  \
  {                                                                                                                                              \
    const d_token_t* val = d_get_at(params, index);                                                                                              \
    if (d_type(val) != T_BYTES || val->len != 20) return req_set_error(ctx, "argument at index " #index " must be a valid address", IN3_EINVAL); \
  }

/* checks that the parameter at the given index has exactly the given len*/
#define CHECK_PARAM_LEN(ctx, params, index, len) \
  if (d_len(d_get_at(params, index)) != len) return req_set_error(ctx, "argument at index " #index " must have a length of " #len, IN3_EINVAL);

/* checks that the parameter at the given index has a len greater than the given amount*/
#define CHECK_PARAM_MAX_LEN(ctx, params, index, len) \
  if (d_len(d_get_at(params, index)) > len) return req_set_error(ctx, "argument at index " #index " must be smaller than " #len, IN3_EINVAL);

/* checks that the parameter at the given index has a len less than the given amount*/
#define CHECK_PARAM_MIN_LEN(ctx, params, index, len) \
  if (d_len(d_get_at(params, index)) < len) return req_set_error(ctx, "argument at index " #index " must be at least " #len, IN3_EINVAL);

/* checks that the parameter at the given index fulfills the given condition*/
#define CHECK_PARAM(ctx, params, index, cond)                                                             \
  {                                                                                                       \
    const d_token_t* val = d_get_at(params, index);                                                       \
    if (!(cond)) return req_set_error(ctx, "argument at index " #index " must match " #cond, IN3_EINVAL); \
  }

/* fetches the parameter with the given index as bytes and stores it in the target, which must be a bytes_t variable*/
#define TRY_PARAM_GET_BYTES(target, ctx, index, min_len, max_len)                                                                                         \
  {                                                                                                                                                       \
    d_token_t* t = d_get_at(ctx->params, index);                                                                                                          \
    if (d_type(t) == T_NULL)                                                                                                                              \
      target = NULL_BYTES;                                                                                                                                \
    else if (d_type(t) == T_OBJECT || d_type(t) == T_ARRAY)                                                                                               \
      return req_set_error(ctx->req, "Param at " #index " must be bytes!", IN3_EINVAL);                                                                   \
    else {                                                                                                                                                \
      target = d_to_bytes(t);                                                                                                                             \
      if ((int) target.len < (int) min_len) return req_set_error(ctx->req, "Param at " #index " must have at least a length of " #min_len, IN3_EINVAL);   \
      if (max_len && (int) target.len > (int) max_len) return req_set_error(ctx->req, "Param at " #index " must have max " #max_len "bytes", IN3_EINVAL); \
    }                                                                                                                                                     \
  }

/* fetches the required parameter with the given index as bytes and stores it in the target, which must be a bytes_t variable*/
#define TRY_PARAM_GET_REQUIRED_BYTES(target, ctx, index, min_len, max_len)                                                                              \
  {                                                                                                                                                     \
    target = d_to_bytes(d_get_at(ctx->params, index));                                                                                                  \
    if (!target.data) return req_set_error(ctx->req, "Param at " #index " must be bytes!", IN3_EINVAL);                                                 \
    if ((int) target.len < (int) min_len) return req_set_error(ctx->req, "Param at " #index " must have at least a length of " #min_len, IN3_EINVAL);   \
    if (max_len && (int) target.len > (int) max_len) return req_set_error(ctx->req, "Param at " #index " must have max " #max_len "bytes", IN3_EINVAL); \
  }

/* fetches the parameter with the given index as integer and stores it in the target, which must be a int or int32_t variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_INT(target, ctx, index, def)                                                 \
  {                                                                                                \
    const d_token_t* t = d_get_at(ctx->params, index);                                             \
    if (d_type(t) == T_NULL)                                                                       \
      target = def;                                                                                \
    else if (d_type(t) != T_INTEGER)                                                               \
      return req_set_error(ctx->req, "Param at " #index " must be an integer value!", IN3_EINVAL); \
    else                                                                                           \
      target = d_int(t);                                                                           \
  }

/* fetches the required parameter with the given index as integer and stores it in the target, which must be a int or int32_t variable*/
#define TRY_PARAM_GET_REQUIRED_INT(target, ctx, index)                                             \
  {                                                                                                \
    const d_token_t* t = d_get_at(ctx->params, index);                                             \
    if (d_type(t) != T_INTEGER)                                                                    \
      return req_set_error(ctx->req, "Param at " #index " must be an integer value!", IN3_EINVAL); \
    else                                                                                           \
      target = d_int(t);                                                                           \
  }

/* fetches the parameter with the given index as unsigned long integer and stores it in the target, which must be a uint64_t variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_LONG(target, ctx, index, def)                                             \
  {                                                                                             \
    const d_token_t* t = d_get_at(ctx->params, index);                                          \
    if (d_type(t) == T_NULL)                                                                    \
      target = def;                                                                             \
    else if (d_type(t) == T_INTEGER || (d_type(t) == T_BYTES && d_len(t) <= 8))                 \
      target = d_long(t);                                                                       \
    else                                                                                        \
      return req_set_error(ctx->req, "Param at " #index " must be an long value!", IN3_EINVAL); \
  }

/* fetches the required parameter with the given index as unsigned long integer and stores it in the target, which must be a uint64_t variable*/
#define TRY_PARAM_GET_REQUIRED_LONG(target, ctx, index)                                         \
  {                                                                                             \
    const d_token_t* t = d_get_at(ctx->params, index);                                          \
    if (d_type(t) == T_INTEGER || (d_type(t) == T_BYTES && d_len(t) <= 8))                      \
      target = d_long(t);                                                                       \
    else                                                                                        \
      return req_set_error(ctx->req, "Param at " #index " must be an long value!", IN3_EINVAL); \
  }

/* fetches the parameter with the given index as boolean and stores it in the target, which must be a bool or int variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_BOOL(target, ctx, index, def)                                                \
  {                                                                                                \
    const d_token_t* t = d_get_at(ctx->params, index);                                             \
    if (d_type(t) == T_NULL)                                                                       \
      target = def;                                                                                \
    else if (d_type(t) != T_BOOLEAN)                                                               \
      return req_set_error(ctx->req, "Param at " #index " must be an true or false!", IN3_EINVAL); \
    else                                                                                           \
      target = d_int(t);                                                                           \
  }

/* fetches the parameter with the given index as address (20 bytes) and stores it in the target, which must be a uint8_t* variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_ADDRESS(target, ctx, index, def)                                            \
  {                                                                                               \
    const d_token_t* t = d_get_at(ctx->params, index);                                            \
    if (d_type(t) == T_NULL)                                                                      \
      target = def;                                                                               \
    else if (d_type(t) != T_BYTES || d_len(t) != 20)                                              \
      return req_set_error(ctx->req, "Param at " #index " must be a valid address!", IN3_EINVAL); \
    else                                                                                          \
      target = t->data;                                                                           \
  }

/* fetches the required parameter with the given index as address (20 bytes) and stores it in the target, which must be a uint8_t* variable*/
#define TRY_PARAM_GET_REQUIRED_ADDRESS(target, ctx, index)                                        \
  {                                                                                               \
    const d_token_t* t = d_get_at(ctx->params, index);                                            \
    if (d_type(t) != T_BYTES || d_len(t) != 20)                                                   \
      return req_set_error(ctx->req, "Param at " #index " must be a valid address!", IN3_EINVAL); \
    else                                                                                          \
      target = t->data;                                                                           \
  }

/* fetches the parameter with the given index as string and stores it in the target, which must be a char* variable. If a the last arg(def) is not NULL, it will be used as default if the parameter is not set.*/
#define TRY_PARAM_GET_STRING(target, ctx, index, def)                                              \
  {                                                                                                \
    const d_token_t* t = d_get_at(ctx->params, index);                                             \
    switch (d_type(t)) {                                                                           \
      case T_NULL:                                                                                 \
        target = def;                                                                              \
        break;                                                                                     \
      case T_BYTES:                                                                                \
        target = alloca(t->len * 2 + 3);                                                           \
        bytes_to_hex_string(target, "0x", *d_bytes(t), NULL);                                      \
        break;                                                                                     \
      case T_INTEGER:                                                                              \
        target = alloca(10);                                                                       \
        sprintf(target, "%d", d_int(t));                                                           \
        break;                                                                                     \
      case T_STRING:                                                                               \
        target = d_string(t);                                                                      \
        break;                                                                                     \
      default:                                                                                     \
        return req_set_error(ctx->req, "Param at " #index " must be a valid string!", IN3_EINVAL); \
    }                                                                                              \
  }

/* fetches the required parameter with the given index as string and stores it in the target, which must be a char* variable.*/
#define TRY_PARAM_GET_REQUIRED_STRING(target, ctx, index)                                                   \
  {                                                                                                         \
    TRY_PARAM_GET_STRING(target, ctx, index, NULL)                                                          \
    if (!target) return req_set_error(ctx->req, "Param at " #index " must be a valid string!", IN3_EINVAL); \
  }

/* fetches the parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json object, but allows NULL, if it is not given.*/
#define TRY_PARAM_GET_OBJECT(target, ctx, index)                                                 \
  {                                                                                              \
    target = d_get_at(ctx->params, index);                                                       \
    if (d_type(target) == T_NULL)                                                                \
      target = NULL;                                                                             \
    else if (d_type(target) != T_OBJECT)                                                         \
      return req_set_error(ctx->req, "Param at " #index " must be a valid object!", IN3_EINVAL); \
  }

/* fetches the required parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json object.*/
#define TRY_PARAM_GET_REQUIRED_OBJECT(target, ctx, index)                                        \
  {                                                                                              \
    target = d_get_at(ctx->params, index);                                                       \
    if (d_type(target) != T_OBJECT)                                                              \
      return req_set_error(ctx->req, "Param at " #index " must be a valid object!", IN3_EINVAL); \
  }

/* fetches the parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json array, but allows NULL, if it is not given.*/
#define TRY_PARAM_GET_ARRAY(target, ctx, index)                                                 \
  {                                                                                             \
    target = d_get_at(ctx->params, index);                                                      \
    if (d_type(target) == T_NULL)                                                               \
      target = NULL;                                                                            \
    else if (d_type(target) != T_ARRAY)                                                         \
      return req_set_error(ctx->req, "Param at " #index " must be a valid array!", IN3_EINVAL); \
  }

/* fetches the required parameter with the given index as object and stores it in the target, which must be a d_token_t* variable. It will throw an error if the parameter is not an json array.*/
#define TRY_PARAM_GET_REQUIRED_ARRAY(target, ctx, index)                                        \
  {                                                                                             \
    target = d_get_at(ctx->params, index);                                                      \
    if (d_type(target) != T_ARRAY)                                                              \
      return req_set_error(ctx->req, "Param at " #index " must be a valid array!", IN3_EINVAL); \
  }

#endif