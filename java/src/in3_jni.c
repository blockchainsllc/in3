/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/blockchainsllc/in3
 *
 * Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
 *
 *
 * COMMERCIAL LICENSE USAGE
 *
 * Licensees holding a valid commercial license may use this file in accordance
 * with the commercial license agreement provided with the Software or, alternatively,
 * in accordance with the terms contained in a written agreement between you and
 * slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further
 * information please contact slock.it at in3@slock.it.
 *
 * Alternatively, this file may be used under the AGPL license as follows:
 *
 * AGPL LICENSE USAGE
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 * [Permissions of this strong copyleft license are conditioned on making available
 * complete source code of licensed works and modifications, which include larger
 * works using a licensed work, under the same license. Copyright and license notices
 * must be preserved. Contributors provide an express grant of patent rights.]
 * You should have received a copy of the GNU Affero General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#include "in3_jni.h"
#include "../../c/src/api/eth1/abi.h"
#include "../../c/src/api/eth1/eth_api.h"
#include "../../c/src/core/client/client.h"
#include "../../c/src/core/client/keys.h"
#include "../../c/src/core/client/plugin.h"
#include "../../c/src/core/client/request_internal.h"
#include "../../c/src/core/client/version.h"
#include "../../c/src/core/util/bitset.h"
#include "../../c/src/core/util/bytes.h"
#include "../../c/src/core/util/log.h"
#include "../../c/src/core/util/mem.h"
#include "../../c/src/init/in3_init.h"
#include "../../c/src/nodeselect/full/cache.h"
#include "../../c/src/nodeselect/full/nodeselect_def.h"
#include "../../c/src/signer/pk-signer/signer.h"
#include "../../c/src/third-party/crypto/ecdsa.h"
#include "../../c/src/third-party/crypto/secp256k1.h"
#include "../../c/src/verifier/eth1/basic/eth_basic.h"

#ifdef BASE64
#include "../../c/src/third-party/libb64/cdecode.h"
#include "../../c/src/third-party/libb64/cencode.h"
#endif
#if IPFS
#include "../../c/src/verifier/ipfs/ipfs.h"
#endif

typedef struct in3_storage_handler {
  in3_storage_get_item get_item; /**< function pointer returning a stored value for the given key.*/
  in3_storage_set_item set_item; /**< function pointer setting a stored value for the given key.*/
  in3_storage_clear    clear;    /**< function pointer clearing all contents of cache.*/
  void*                cptr;     /**< custom pointer which will be passed to functions */
} in3_storage_handler_t;

static void* get_java_obj_ptr(in3_t* c) {
  in3_log_debug(":: get_java_obj_ptr %p \n", c);
  if (!c) return NULL;
  for (in3_plugin_t* p = c->plugins; p; p = p->next) {
    if (p->acts & PLGN_ACT_CACHE_GET) {
      in3_storage_handler_t* st = p->data;
      in3_log_debug(":: found CACHE %p \n", st);
      return st ? st->cptr : NULL;
    }
  }
  return NULL;
}

static in3_t* get_in3(JNIEnv* env, jobject obj) {
  if (obj == NULL || env == NULL || (*env)->GetObjectClass(env, obj) == NULL) return NULL;
  jlong l = (*env)->GetLongField(env, obj, (*env)->GetFieldID(env, (*env)->GetObjectClass(env, obj), "ptr", "J"));
  return (in3_t*) (size_t) l;
}

/*
 * Class:     in3_IN3
 * Method:    setConfig
 * Signature: (Ljava/lang/String)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setConfig(JNIEnv* env, jobject ob, jstring val) {
  const char* json_config = (*env)->GetStringUTFChars(env, val, 0);
  char*       error       = in3_configure(get_in3(env, ob), json_config);
  (*env)->ReleaseStringUTFChars(env, val, json_config);
  if (error) {
    jclass IllegalArgumentException = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    (*env)->ThrowNew(env, IllegalArgumentException, error);
    _free(error);
  }
}

/*
 * Class:     in3_IN3
 * Method:    getChainId
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_in3_IN3_getChainId(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->chain.id;
}

/*
 * Class:     in3_IN3
 * Method:    setChainId
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setChainId(JNIEnv* env, jobject ob, jlong val) {
  get_in3(env, ob)->chain.id = val;
}

static JNIEnv* jni = NULL;

static jobject get_storage_handler(void* cptr) {
  if (!jni || !cptr) return NULL;
  jclass    cls = (*jni)->GetObjectClass(jni, (jobject) cptr);
  jmethodID mid = (*jni)->GetMethodID(jni, cls, "getStorageProvider", "()Lin3/utils/StorageProvider;");
  return (*jni)->CallObjectMethod(jni, (jobject) cptr, mid);
}

bytes_t* storage_get_item(void* cptr, const char* key) {
  jobject handler = get_storage_handler(cptr);
  if (!handler) return NULL;

  jstring    js     = (*jni)->NewStringUTF(jni, key);
  jbyteArray result = (jbyteArray) (*jni)->CallObjectMethod(jni, handler,
                                                            (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, handler), "getItem", "(Ljava/lang/String;)[B"),
                                                            js);
  if (result == NULL) return NULL;

  bytes_t* res = _malloc(sizeof(bytes_t));
  res->len     = (*jni)->GetArrayLength(jni, result);
  res->data    = _malloc(res->len);
  (*jni)->GetByteArrayRegion(jni, result, 0, res->len, (jbyte*) res->data);

  return res;
}

void storage_set_item(void* cptr, const char* key, bytes_t* content) {
  jobject handler = get_storage_handler(cptr);
  if (!handler) return;

  jbyteArray bytes = (*jni)->NewByteArray(jni, content->len);
  (*jni)->SetByteArrayRegion(jni, bytes, 0, content->len, (jbyte*) content->data);
  (*jni)->CallObjectMethod(jni, handler, (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, handler), "setItem", "(Ljava/lang/String;[B)V"), (*jni)->NewStringUTF(jni, key), bytes);
}

void storage_clear(void* cptr) {
  jobject handler = get_storage_handler(cptr);
  if (!handler) return;

  (*jni)->CallObjectMethod(jni, handler, (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, handler), "clear", "(V)B"));
}

JNIEXPORT void JNICALL Java_in3_IN3_initcache(JNIEnv* env, jobject ob) {
  in3_t* c = get_in3(env, ob);
  in3_cache_init(c, in3_nodeselect_def_data(c));
}

/*
 * Class:     in3_IN3
 * Method:    sendinternal
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_IN3_sendinternal(JNIEnv* env, jobject ob, jstring jreq) {
  jni = env;

  const char* str    = (*env)->GetStringUTFChars(env, jreq, 0);
  char*       result = NULL;
  char        error[10000];
  int         res;
  jstring     js = NULL;

  in3_req_t* ctx = req_new(get_in3(env, ob), (char*) str);

  if (!ctx->error) {
    res = in3_send_req(ctx);
    if (res >= 0) {
      d_token_t* r = d_get(ctx->responses[0], K_RESULT);
      if (r)
        result = d_create_json(ctx->response_context, r);
      else if ((r = d_get(ctx->responses[0], K_ERROR))) {
        if (d_type(r) == T_OBJECT) {
          str_range_t s = d_to_json(r);
          strncpy(error, s.data, s.len);
          error[s.len] = '\0';
        }
        else {
          strncpy(error, d_string(r), d_len(r));
          error[d_len(r)] = '\0';
        }
      }
      else if (ctx->error)
        strcpy(error, ctx->error);
      else
        strcpy(error, "No Result and also no error");
    }
    else if (ctx->error)
      strcpy(error, ctx->error);
    else
      strcpy(error, "Error sending the request");
  }
  else
    strcpy(error, ctx->error);

  // need to release this string when done with it in order to
  // avoid memory leak
  (*env)->ReleaseStringUTFChars(env, jreq, str);
  req_free(ctx);

  if (result) {
    js = (*env)->NewStringUTF(env, result);
    _free(result);
    return js;
  }
  else {
    jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
    (*env)->ThrowNew(env, Exception, error);
  }
  return js;
}

static jobject toObject(JNIEnv* env, d_token_t* t) {
  jclass clz;
  switch (d_type(t)) {
    case T_NULL:
      return NULL;
    case T_BOOLEAN:
      clz = (*env)->FindClass(env, "java/lang/Boolean");
      return (*env)->CallStaticObjectMethod(env, clz, (*env)->GetStaticMethodID(env, clz, "valueOf", "(Z)Ljava/lang/Boolean;"), (bool) d_int(t));
    case T_INTEGER:
      clz = (*env)->FindClass(env, "java/lang/Integer");
      return (*env)->CallStaticObjectMethod(env, clz, (*env)->GetStaticMethodID(env, clz, "valueOf", "(I)Ljava/lang/Integer;"), d_int(t));
    case T_STRING:
      return (*env)->NewStringUTF(env, d_string(t));
    case T_BYTES: {
      char* tmp = alloca(t->len * 2 + 3);
      tmp[0]    = '0';
      tmp[1]    = 'x';
      bytes_to_hex(t->data, t->len, tmp + 2);
      return (*env)->NewStringUTF(env, tmp);
    }
    case T_OBJECT: {
      clz           = (*env)->FindClass(env, "in3/utils/JSON");
      jobject   map = (*env)->NewObject(env, clz, (*env)->GetMethodID(env, clz, "<init>", "()V"));
      jmethodID put = (*env)->GetMethodID(env, clz, "put", "(ILjava/lang/Object;)V");
      for (d_iterator_t iter = d_iter(t); iter.left; d_iter_next(&iter))
        (*env)->CallVoidMethod(env, map, put, iter.token->key, toObject(env, iter.token));
      return map;
    }
    case T_ARRAY: {
      jobject array = (*env)->NewObjectArray(env, d_len(t), (*env)->FindClass(env, "java/lang/Object"), NULL);
      for (int i = 0; i < d_len(t); i++) (*env)->SetObjectArrayElement(env, array, i, toObject(env, d_get_at(t, i)));
      return array;
    }
  }
  return NULL;
}

JNIEXPORT jint JNICALL Java_in3_utils_JSON_key(JNIEnv* env, jclass ob, jstring k) {
  jint        val = 0;
  const char* str = (*env)->GetStringUTFChars(env, k, 0);
  val             = key(str);
  (*env)->ReleaseStringUTFChars(env, k, str);
  return ob ? val : 0;
}

/*
 * Class:     in3_IN3
 * Method:    Java_in3_IN3_sendobjectinternal
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jobject JNICALL Java_in3_IN3_sendobjectinternal(JNIEnv* env, jobject ob, jstring jreq) {
  jni = env;

  const char* str    = (*env)->GetStringUTFChars(env, jreq, 0);
  d_token_t*  result = NULL;
  char        error[10000];
  int         res;
  jobject     js = NULL;

  in3_req_t* ctx = req_new(get_in3(env, ob), (char*) str);

  if (!ctx->error) {
    res = in3_send_req(ctx);
    if (res >= 0) {
      d_token_t* r = d_get(ctx->responses[0], K_RESULT);
      if (r)
        js = toObject(env, result = r);
      else if ((r = d_get(ctx->responses[0], K_ERROR))) {
        if (d_type(r) == T_OBJECT) {
          str_range_t s = d_to_json(r);
          strncpy(error, s.data, s.len);
          error[s.len] = '\0';
        }
        else {
          strncpy(error, d_string(r), d_len(r));
          error[d_len(r)] = '\0';
        }
      }
      else if (ctx->error)
        strcpy(error, ctx->error);
      else
        strcpy(error, "No Result and also no error");
    }
    else if (ctx->error)
      strcpy(error, ctx->error);
    else
      strcpy(error, "Error sending the request");
  }
  else
    strcpy(error, ctx->error);

  // need to release this string when done with it in order to
  // avoid memory leak
  (*env)->ReleaseStringUTFChars(env, jreq, str);

  req_free(ctx);

  if (result)
    return js;
  else {
    jclass Exception = (*env)->FindClass(env, "java/lang/Exception");
    (*env)->ThrowNew(env, Exception, error);
  }
  return NULL;
}

/*
 * Class:     in3_IN3
 * Method:    free
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_in3_IN3_free(JNIEnv* env, jobject ob) {
  in3_t* in3 = get_in3(env, ob);
  if (!in3) return;
  void* jp = get_java_obj_ptr(in3);
  if (jp)
    (*env)->DeleteGlobalRef(env, (jobject) jp);

  in3_free(in3);
}

in3_ret_t Java_in3_IN3_transport(void* plugin_data, in3_plugin_act_t action, void* plugin_ctx) {
  UNUSED_VAR(plugin_data);
  UNUSED_VAR(action);

  in3_http_request_t* req   = plugin_ctx;
  uint64_t            start = current_ms();
  // char** urls, int urls_len, char* payload, in3_response_t* res
  in3_ret_t success = IN3_OK;
  // payload
  jbyteArray jpayload = (*jni)->NewByteArray(jni, req->payload_len);
  (*jni)->SetByteArrayRegion(jni, jpayload, 0, req->payload_len, (jbyte*) req->payload);

  // url-array
  jobject jurls = (*jni)->NewObjectArray(jni, req->urls_len, (*jni)->FindClass(jni, "java/lang/String"), NULL);
  for (unsigned int i = 0; i < req->urls_len; i++) (*jni)->SetObjectArrayElement(jni, jurls, i, (*jni)->NewStringUTF(jni, req->urls[i]));

  // headers
  int header_len = 0, hi = 0;
  for (in3_req_header_t* h = req->headers; h; h = h->next) header_len++;
  jstring jmethod  = (*jni)->NewStringUTF(jni, req->method);
  jobject jheaders = (*jni)->NewObjectArray(jni, header_len, (*jni)->FindClass(jni, "java/lang/String"), NULL);
  for (in3_req_header_t* h = req->headers; h; h = h->next, hi++) (*jni)->SetObjectArrayElement(jni, jheaders, hi, (*jni)->NewStringUTF(jni, h->value));

  (*jni)->ExceptionClear(jni);
  jclass       cls    = (*jni)->FindClass(jni, "in3/IN3");
  jmethodID    mid    = (*jni)->GetStaticMethodID(jni, cls, "sendRequest", "(Ljava/lang/String;[Ljava/lang/String;[B[Ljava/lang/String;)[[B");
  jobjectArray result = (*jni)->CallStaticObjectMethod(jni, cls, mid, jmethod, jurls, jpayload, jheaders);
  uint64_t     end    = current_ms();

  // handle exception
  jthrowable transport_exception = (*jni)->ExceptionOccurred(jni);
  if (transport_exception) {
    jclass    cls    = (*jni)->GetObjectClass(jni, transport_exception);
    jmethodID mid    = (*jni)->GetMethodID(jni, cls, "getStatus", "()I");
    int       status = (*jni)->CallIntMethod(jni, transport_exception, mid);
    mid              = (*jni)->GetMethodID(jni, cls, "getIndex", "()I");
    int index        = (*jni)->CallIntMethod(jni, transport_exception, mid);
    mid              = (*jni)->GetMethodID(jni, cls, "getMessage", "()Ljava/lang/String;");
    jstring     jmsg = (*jni)->CallObjectMethod(jni, transport_exception, mid);
    const char* msg  = (*jni)->GetStringUTFChars(jni, jmsg, 0);
    in3_req_add_response(req, index, 0 - status, msg, -1, (uint32_t) (end - start));
    (*jni)->ReleaseStringUTFChars(jni, jmsg, msg);
    (*jni)->ExceptionClear(jni);
  }
  else {
    for (unsigned int i = 0; i < req->urls_len; i++) {
      jbyteArray content = result ? (*jni)->GetObjectArrayElement(jni, result, i) : NULL;
      if (content) {
        const size_t l     = (*jni)->GetArrayLength(jni, content);
        uint8_t*     bytes = _malloc(l);
        (*jni)->GetByteArrayRegion(jni, content, 0, l, (jbyte*) bytes);
        sb_add_range(&req->req->raw_response[i].data, (char*) bytes, 0, l);
        req->req->raw_response[i].state = IN3_OK;
        _free(bytes);
      }
      else {
        sb_add_chars(&req->req->raw_response[i].data, "Could not fetch the data!");
        req->req->raw_response[i].state = IN3_ERPC;
      }
      if (req->req->raw_response[i].state) success = IN3_ERPC;
    }
  }

  for (unsigned int i = 0; i < req->urls_len; i++) req->req->raw_response[i].time = (uint32_t) (end - start);

  return success;
}

/*
 * Class:     in3_eth1_TransactionRequest
 * Method:    abiEncode
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_eth1_TransactionRequest_abiEncode(JNIEnv* env, jclass clz, jstring fn, jstring json) {
  UNUSED_VAR(clz);
  char*       error = NULL;
  const char* fnc   = (*env)->GetStringUTFChars(env, fn, 0);
  abi_sig_t*  rq    = abi_sig_create((char*) fnc, &error);
  (*env)->ReleaseStringUTFChars(env, fn, fnc);
  if (error) {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), error);
    if (rq) abi_sig_free(rq);
    return NULL;
  }

  const char* json_data = (*env)->GetStringUTFChars(env, json, 0);
  json_ctx_t* json_ctx  = parse_json((char*) json_data);
  (*env)->ReleaseStringUTFChars(env, json, json_data);

  if (!json_ctx) {
    abi_sig_free(rq);
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "Error parsing the data");
    return NULL;
  }

  bytes_t data = abi_encode(rq, json_ctx->result, &error);
  abi_sig_free(rq);
  json_free(json_ctx);
  if (error) {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), error);
    return NULL;
  }

  jstring res = (jstring) toObject(env, (d_token_t*) &data);
  _free(data.data);
  return res;
}

/*
 * Class:     in3_eth1_TransactionRequest
 * Method:    abiDecode
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Lin3/JSON;
 */
JNIEXPORT jobject JNICALL Java_in3_eth1_TransactionRequest_abiDecode(JNIEnv* env, jclass clz, jstring fn, jstring data) {
  UNUSED_VAR(clz);
  char*       error = NULL;
  const char* fnc   = (*env)->GetStringUTFChars(env, fn, 0);
  abi_sig_t*  rq    = abi_sig_create((char*) fnc, &error);
  (*env)->ReleaseStringUTFChars(env, fn, fnc);
  if (error) {
    if (rq) abi_sig_free(rq);
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), error);
    return NULL;
  }

  const char* jdata = (*env)->GetStringUTFChars(env, data, 0);
  int         l     = strlen(jdata);
  uint8_t*    bdata = alloca(l >> 1);
  l                 = hex_to_bytes((char*) jdata + 2, l - 2, bdata, l);
  (*env)->ReleaseStringUTFChars(env, data, jdata);

  json_ctx_t* res    = abi_decode(rq, bytes(bdata, l), &error);
  jobject     result = res ? toObject(env, res->result) : NULL;
  abi_sig_free(rq);
  if (res)
    json_free(res);
  else
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), error);

  return result;
}

/*
 * Class:     in3_utils_Signer
 * Method:    getAddressFromKey
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_utils_Signer_getAddressFromKey(JNIEnv* env, jclass clz, jstring jkey) {
  UNUSED_VAR(clz);
  const char* key = (*env)->GetStringUTFChars(env, jkey, 0);

  bytes32_t prv_key;
  uint8_t   public_key[65], sdata[32];
  hex_to_bytes((char*) key, -1, prv_key, 32);
  ecdsa_get_public_key65(&secp256k1, prv_key, public_key);
  keccak(bytes(public_key + 1, 64), sdata);
  (*env)->ReleaseStringUTFChars(env, jkey, key);
  char tmp[43];
  bytes_to_hex(sdata + 12, 20, tmp + 2);
  tmp[0] = '0';
  tmp[1] = 'x';
  return (*env)->NewStringUTF(env, tmp);
}

/*
 * Class:     in3_utils_Signer
 * Method:    signData
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jbyteArray JNICALL Java_in3_utils_Signer_signData(JNIEnv* env, jclass clz, jstring jkey, jstring jdata, jobject signatureTypeObj) {
  jbyteArray res = (*env)->NewByteArray(env, 65);

  UNUSED_VAR(clz);
  const char* key    = (*env)->GetStringUTFChars(env, jkey, 0);
  const char* data   = (*env)->GetStringUTFChars(env, jdata, 0);
  int         data_l = strlen(data) / 2 - 1;

  jmethodID sisgnatureTypeGetValueMethod = (*env)->GetMethodID(env, (*env)->FindClass(env, "in3/utils/SignatureType"), "getValue", "()I");
  jint      jSignType                    = (*env)->CallIntMethod(env, signatureTypeObj, sisgnatureTypeGetValueMethod);

  uint8_t key_bytes[32], *data_bytes = alloca(data_l + 1);
  if (data[0] == '0' && data[1] == 'x') {
    data_l = hex_to_bytes((char*) data + 2, -1, data_bytes, data_l + 1);
  }
  else {
    data_l = hex_to_bytes((char*) data, -1, data_bytes, data_l + 1);
  }
  hex_to_bytes((char*) key, -1, key_bytes, 32);

  bytes_t signature = sign_with_pk(key_bytes, bytes(data_bytes, data_l), jSignType);
  if (signature.len == 65)
    (*env)->SetByteArrayRegion(env, res, 0, signature.len, (jbyte*) signature.data);
  if (signature.data) _free(signature.data);

  (*env)->ReleaseStringUTFChars(env, jkey, key);
  (*env)->ReleaseStringUTFChars(env, jdata, data);

  return res;
}

/*
 * Class:     in3_utils_Signer
 * Method:    decodeKeystore
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_utils_Signer_decodeKeystore(JNIEnv* env, jclass clz, jstring json, jstring passphrase) {
  UNUSED_VAR(clz);
  UNUSED_VAR(env);
  UNUSED_VAR(json);
  UNUSED_VAR(passphrase);
  return NULL;
}

jobject get_signer(in3_req_t* ctx) {
  void* jp = get_java_obj_ptr(ctx->client);
  if (jp == NULL) return NULL;
  jclass    cls = (*jni)->GetObjectClass(jni, jp);
  jmethodID mid = (*jni)->GetMethodID(jni, cls, "getSigner", "()Lin3/utils/Signer;");
  return (*jni)->CallObjectMethod(jni, jp, mid);
}

// This assumes a getEnum method which is not natural to every Enum, this will only work with custom ones.
jobject get_enum(const char* qualified_class_name, const char* getter_name, int value) {
  jclass    cls = (*jni)->FindClass(jni, qualified_class_name);
  jmethodID mid = (*jni)->GetStaticMethodID(jni, cls, "getEnum", getter_name);
  return (jobject) (*jni)->CallStaticObjectMethod(jni, cls, mid, value);
}

// in3_ret_t jsign(void* pk, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
in3_ret_t jsign(in3_sign_ctx_t* sc) {
  in3_req_t* ctx = (in3_req_t*) sc->req;
  if (ctx == NULL) return IN3_EIGNORE;
  jobject signer = get_signer(ctx);

  if (!signer || !sc->account.data) return IN3_EIGNORE;

  char *data = alloca(sc->message.len * 2 + 3), address[43];
  data[0] = address[0] = '0';
  data[1] = address[1] = 'x';
  bytes_to_hex(sc->message.data, sc->message.len, data + 2);
  bytes_to_hex(sc->account.data, sc->account.len, address + 2);

  jobject jSignatureType = get_enum("in3/utils/SignatureType", "(I)Lin3/utils/SignatureType;", sc->type);
  jobject jPayloadType   = get_enum("in3/utils/PayloadType", "(I)Lin3/utils/PayloadType;", sc->payload_type);
  jstring jdata          = (*jni)->NewStringUTF(jni, data);
  jstring jaddress       = (*jni)->NewStringUTF(jni, address);
  jclass  cls            = (*jni)->GetObjectClass(jni, signer);

  jmethodID mid = (*jni)->GetMethodID(jni, cls, "sign", "(Ljava/lang/String;Ljava/lang/String;Lin3/utils/SignatureType;Lin3/utils/PayloadType;Lin3/utils/JSON;)[B");

  (*jni)->ExceptionClear(jni);
  jbyteArray jsignature        = (*jni)->CallObjectMethod(jni, signer, mid, jdata, jaddress, jSignatureType, jPayloadType, toObject(jni, sc->meta));
  jthrowable signing_exception = (*jni)->ExceptionOccurred(jni);

  if (signing_exception) {
    jclass cls   = (*jni)->GetObjectClass(jni, signing_exception);
    mid          = (*jni)->GetMethodID(jni, cls, "getMessage", "()Ljava/lang/String;");
    jstring jmsg = (*jni)->CallObjectMethod(jni, signing_exception, mid);
    char*   msg  = (char*) (*jni)->GetStringUTFChars(jni, jmsg, 0);

    (*jni)->ExceptionClear(jni);
    return req_set_error(sc->req, msg, IN3_ERPC);
  }

  if (!jsignature) return IN3_EIGNORE;

  int l         = (*jni)->GetArrayLength(jni, jsignature);
  sc->signature = bytes(_malloc(l), l);
  (*jni)->GetByteArrayRegion(jni, jsignature, 0, l, (jbyte*) sc->signature.data);

  return IN3_OK;
}

// in3_ret_t jsign(void* pk, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
in3_ret_t jsign_accounts(in3_sign_account_ctx_t* sc) {
  in3_req_t* ctx = (in3_req_t*) sc->req;
  if (ctx == NULL) return IN3_EIGNORE;

  jobject signer = get_signer(ctx);
  if (signer == NULL) return IN3_EIGNORE;

  jclass       cls            = (*jni)->GetObjectClass(jni, signer);
  jmethodID    mid            = (*jni)->GetMethodID(jni, cls, "getAccounts", "()[Ljava/lang/String;");
  jobjectArray jaccounts      = (*jni)->CallObjectMethod(jni, signer, mid);
  int          accounts_total = (*jni)->GetArrayLength(jni, jaccounts);

  if (!jaccounts || accounts_total == 0) return IN3_EIGNORE;

  // This assumption is incorrect as anyone could just implement the signer.
  sc->accounts_len = accounts_total * 20;
  sc->accounts     = _malloc(accounts_total * 20);

  for (int i = 0; i < accounts_total; ++i) {
    jstring jaccount = (*jni)->GetObjectArrayElement(jni, jaccounts, i);
    char*   account  = (char*) (*jni)->GetStringUTFChars(jni, jaccount, 0);
    hex_to_bytes(account, -1, sc->accounts + i * 20, 20);
    (*jni)->ReleaseStringUTFChars(jni, jaccount, account);
  }

  return IN3_OK;
}

JNIEXPORT jobject Java_in3_IN3_getDefaultConfig(JNIEnv* env, jobject ob) {
  UNUSED_VAR(ob);

  char*       ret  = in3_get_config(get_in3(env, ob));
  json_ctx_t* json = parse_json(ret);
  d_token_t*  r    = &json->result[0];
  jobject     res  = toObject(env, r);

  _free(ret);
  json_free(json);

  return res;
}

#ifdef BASE64
/*
 * Class:     in3_ipfs_API
 * Method:    base64Decode
 * Signature: (Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_in3_ipfs_API_base64Decode(JNIEnv* env, jobject ob, jstring jinput) {
  UNUSED_VAR(ob);
  size_t      len   = 0;
  const char* input = (*env)->GetStringUTFChars(env, jinput, 0);
  uint8_t*    b64   = base64_decode(input, &len);
  (*env)->ReleaseStringUTFChars(env, jinput, input);
  jbyteArray res = (*env)->NewByteArray(env, len);
  (*env)->SetByteArrayRegion(env, res, 0, len, (jbyte*) b64);
  return res;
}

/*
 * Class:     in3_ipfs_API
 * Method:    base64Encode
 * Signature: ([B)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_ipfs_API_base64Encode(JNIEnv* env, jobject ob, jbyteArray jinput) {
  UNUSED_VAR(ob);
  jbyte* body = (*env)->GetByteArrayElements(env, jinput, 0);
  char*  b64  = base64_encode((uint8_t*) body, (*env)->GetArrayLength(env, jinput));
  (*env)->ReleaseByteArrayElements(env, jinput, body, 0);
  jstring jresult = (*env)->NewStringUTF(env, b64);
  return jresult;
}
#endif

static in3_ret_t jsign_fn(void* data, in3_plugin_act_t action, void* ctx) {
  UNUSED_VAR(data);
  switch (action) {
    case PLGN_ACT_SIGN: return jsign(ctx);
    case PLGN_ACT_SIGN_ACCOUNT: return jsign_accounts(ctx);
    default: return IN3_ENOTSUP;
  }
}

/*
 * Class:     in3_IN3
 * Method:    init
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_in3_IN3_init(JNIEnv* env, jobject ob, jlong jchain) {
  in3_init();
  in3_t* in3 = in3_for_chain(jchain);
  void*  p   = (*env)->NewGlobalRef(env, ob);
  // in3_log_set_level(LOG_TRACE);
  // in3_log_set_quiet(false);
  in3_log_debug("New Global ref for %p === %p\n", ob, p);
  in3_set_storage_handler(in3, storage_get_item, storage_set_item, storage_clear, p);
  in3_plugin_register(in3, PLGN_ACT_TRANSPORT, Java_in3_IN3_transport, NULL, true);
  in3_plugin_register(in3, PLGN_ACT_SIGN | PLGN_ACT_SIGN_ACCOUNT, jsign_fn, p, false);
  jni = env;
  // turn to debug

  return (jlong) (size_t) in3;
}

JNIEXPORT jstring JNICALL Java_in3_IN3_getVersion(JNIEnv* env, jclass c) {
  UNUSED_VAR(c);
  return (*env)->NewStringUTF(env, IN3_VERSION);
}

/*
 * Class:     in3_Loader
 * Method:    libInit
 * Signature: ()J
 */
JNIEXPORT void JNICALL Java_in3_Loader_libInit(JNIEnv* env, jclass c) {
  UNUSED_VAR(env);
  UNUSED_VAR(c);
  in3_init();
}

/*
 * Class:     in3_utils_JSON
 * Method:    parse
 * Signature: (Ljava/lang/String;)Lin3/JSON;
 */
JNIEXPORT jobject JNICALL Java_in3_utils_JSON_parse(JNIEnv* env, jclass cl, jstring jdata) {
  UNUSED_VAR(cl);
  jobject     ob   = NULL;
  const char* data = (*env)->GetStringUTFChars(env, jdata, 0);
  json_ctx_t* ctx  = parse_json(data);
  (*env)->ReleaseStringUTFChars(env, jdata, data);
  if (ctx == NULL) {
    char* error = _malloc(strlen(data) + 50);
    sprintf(error, "Error parsing the json-data : '%s'", data);
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/RuntimeException"), error);
    _free(error);
  }
  else {
    ob = toObject(env, ctx->result);
    json_free(ctx);
  }

  return ob;
}
