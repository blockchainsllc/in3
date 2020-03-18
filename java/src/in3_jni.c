/*******************************************************************************
 * This file is part of the Incubed project.
 * Sources: https://github.com/slockit/in3-c
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
#include "../../c/src/core/client/cache.h"
#include "../../c/src/core/client/client.h"
#include "../../c/src/core/client/context.h"
#include "../../c/src/core/client/keys.h"
#include "../../c/src/core/client/version.h"
#include "../../c/src/core/util/bitset.h"
#include "../../c/src/core/util/log.h"
#include "../../c/src/core/util/mem.h"
#include "../../c/src/third-party/crypto/ecdsa.h"
#include "../../c/src/third-party/crypto/secp256k1.h"
#include "../../c/src/verifier/in3_init.h"
#ifdef IPFS
#include "../../c/src/third-party/libb64/cdecode.h"
#include "../../c/src/third-party/libb64/cencode.h"
#include "../../c/src/verifier/ipfs/ipfs.h"
#endif

static in3_t* get_in3(JNIEnv* env, jobject obj) {
  if (obj == NULL || env == NULL || (*env)->GetObjectClass(env, obj) == NULL) return NULL;
  jlong l = (*env)->GetLongField(env, obj, (*env)->GetFieldID(env, (*env)->GetObjectClass(env, obj), "ptr", "J"));
  return (in3_t*) (size_t) l;
}

/*
 * Class:     in3_IN3
 * Method:    getCacheTimeout
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getCacheTimeout(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->cache_timeout;
}

/*s
 * Class:     in3_IN3
 * Method:    setCacheTimeout
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setCacheTimeout(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->cache_timeout = val;
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
    // TODO create a human readable error message
    jclass IllegalArgumentException = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    (*env)->ThrowNew(env, IllegalArgumentException, error);
    _free(error);
  }
}
/*
 * Class:     in3_IN3
 * Method:    getNodeLimit
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getNodeLimit(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->node_limit;
}

/*
 * Class:     in3_IN3
 * Method:    setNodeLimit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setNodeLimit(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->node_limit = val;
}

/*
 * Class:     in3_IN3
 * Method:    getKey
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_in3_IN3_getKey(JNIEnv* env, jobject ob) {
  bytes_t* k = get_in3(env, ob)->key;
  if (!k) return NULL;
  jbyteArray res = (*env)->NewByteArray(env, k->len);
  (*env)->SetByteArrayRegion(env, res, 0, k->len, (jbyte*) k->data);
  return res;
}
/*
 * Class:     in3_IN3
 * Method:    setKey
 * Signature: ([B)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setKey(JNIEnv* env, jobject ob, jbyteArray val) {
  in3_t* in3 = get_in3(env, ob);
  if (in3->key) b_free(in3->key);
  in3->key = NULL;
  if (val == NULL) return;
  int len = (*env)->GetArrayLength(env, val);
  if (len > 32) (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Exception"), "Invalid Signer key!");
  in3->key = _calloc(1, 32);
  (*env)->GetByteArrayRegion(env, val, 0, len, (jbyte*) in3->key);
}

/*
 * Class:     in3_IN3
 * Method:    getMaxCodeCache
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getMaxCodeCache(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->max_code_cache;
}

/*
 * Class:     in3_IN3
 * Method:    setMaxCodeCache
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setMaxCodeCache(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->max_code_cache = val;
}

/*
 * Class:     in3_IN3
 * Method:    getMaxBlockCache
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getMaxBlockCache(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->max_block_cache;
}

/*
 * Class:     in3_IN3
 * Method:    setMaxBlockCache
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setMaxBlockCache(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->max_block_cache = val;
}

/*
 * Class:     in3_IN3
 * Method:    getProof
 * Signature: ()Lin3/Proof;
 */
JNIEXPORT jobject JNICALL Java_in3_IN3_getProof(JNIEnv* env, jobject ob) {
  in3_t*   in3        = get_in3(env, ob);
  jfieldID val        = NULL;
  jclass   enum_clazz = (*env)->FindClass(env, "in3/Proof");
  switch (in3->proof) {
    case PROOF_NONE:
      val = (*env)->GetStaticFieldID(env, enum_clazz, "none", "Lin3/Proof;");
      break;
    case PROOF_STANDARD:
      val = (*env)->GetStaticFieldID(env, enum_clazz, "standard", "Lin3/Proof;");
      break;
    case PROOF_FULL:
      val = (*env)->GetStaticFieldID(env, enum_clazz, "full", "Lin3/Proof;");
      break;
  }
  return (*env)->GetStaticObjectField(env, enum_clazz, val);
}

/*
 * Class:     in3_IN3
 * Method:    setProof
 * Signature: (Lin3/Proof;)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setProof(JNIEnv* env, jobject ob, jobject val) {
  in3_t* in3        = get_in3(env, ob);
  jclass enum_clazz = (*env)->FindClass(env, "in3/Proof");

  char* values[] = {"none", "standard", "full"};
  for (int i = 0; i < 3; i++) {
    if (val == (*env)->GetStaticObjectField(env, enum_clazz, (*env)->GetStaticFieldID(env, enum_clazz, values[i], "Lin3/Proof;")))
      in3->proof = i;
  }
}

/*
 * Class:     in3_IN3
 * Method:    getRequestCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getRequestCount(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->request_count;
}
/*
 * Class:     in3_IN3
 * Method:    setRequestCount
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setRequestCount(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->request_count = val;
}

/*
 * Class:     in3_IN3
 * Method:    getSignatureCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getSignatureCount(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->signature_count;
}

/*
 * Class:     in3_IN3
 * Method:    setSignatureCount
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setSignatureCount(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->signature_count = val;
}

/*
 * Class:     in3_IN3
 * Method:    getMinDeposit
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_in3_IN3_getMinDeposit(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->min_deposit;
}

/*
 * Class:     in3_IN3
 * Method:    setMinDeposit
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setMinDeposit(JNIEnv* env, jobject ob, jlong val) {
  get_in3(env, ob)->min_deposit = val;
}

/*
 * Class:     in3_IN3
 * Method:    getReplaceLatestBlock
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getReplaceLatestBlock(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->replace_latest_block;
}

/*
 * Class:     in3_IN3
 * Method:    getReplaceLatestBlock
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setReplaceLatestBlock(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->replace_latest_block = val;
}

/*
 * Class:     in3_IN3
 * Method:    getFinality
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getFinality(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->finality;
}

/*
 * Class:     in3_IN3
 * Method:    setFinality
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setFinality(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->finality = val;
}

/*
 * Class:     in3_IN3
 * Method:    getMaxAttempts
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getMaxAttempts(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->max_attempts;
}

/*
 * Class:     in3_IN3
 * Method:    setMaxAttempts
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setMaxAttempts(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->max_attempts = val;
}

/*
 * Class:     in3_IN3
 * Method:    getTimeout
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getTimeout(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->timeout;
}

/*
 * Class:     in3_IN3
 * Method:    setTimeout
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setTimeout(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->timeout = val;
}

/*
 * Class:     in3_IN3
 * Method:    getChainId
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_in3_IN3_getChainId(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->chain_id;
}

/*
 * Class:     in3_IN3
 * Method:    setChainId
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setChainId(JNIEnv* env, jobject ob, jlong val) {
  get_in3(env, ob)->chain_id = val;
}

/*
 * Class:     in3_IN3
 * Method:    isAutoUpdateList
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_in3_IN3_isAutoUpdateList(JNIEnv* env, jobject ob) {
  return (get_in3(env, ob)->flags & FLAGS_AUTO_UPDATE_LIST) != 0;
}

/*
 * Class:     in3_IN3
 * Method:    setAutoUpdateList
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setAutoUpdateList(JNIEnv* env, jobject ob, jboolean val) {
  BITMASK_SET_BOOL(get_in3(env, ob)->flags, FLAGS_AUTO_UPDATE_LIST, val);
}

/*
 * Class:     in3_IN3
 * Method:    getStorageProvider
 * Signature: ()Lin3/StorageProvider;
 */
JNIEXPORT jobject JNICALL Java_in3_IN3_getStorageProvider(JNIEnv* env, jobject ob) {
  in3_t* in3 = get_in3(env, ob);
  if (in3->cache && in3->cache->cptr)
    return (jobject) in3->cache->cptr;
  return NULL;
}

static JNIEnv* jni = NULL;

static jobject get_storage_handler(void* cptr) {
  jclass    cls = (*jni)->GetObjectClass(jni, (jobject) cptr);
  jmethodID mid = (*jni)->GetMethodID(jni, cls, "getStorageProvider", "()Lin3/utils/StorageProvider;");
  return (*jni)->CallObjectMethod(jni, (jobject) cptr, mid);
}

bytes_t* storage_get_item(void* cptr, char* key) {
  jobject handler = get_storage_handler(cptr);
  if (!handler) return NULL;

  jstring    js     = (*jni)->NewStringUTF(jni, key);
  jbyteArray result = (jbyteArray)(*jni)->CallObjectMethod(jni, handler,
                                                           (*jni)->GetMethodID(jni, (*jni)->GetObjectClass(jni, handler), "getItem", "(Ljava/lang/String;)[B"),
                                                           js);
  if (result == NULL) return NULL;

  bytes_t* res = _malloc(sizeof(bytes_t));
  res->len     = (*jni)->GetArrayLength(jni, result);
  res->data    = _malloc(res->len);
  (*jni)->GetByteArrayRegion(jni, result, 0, res->len, (jbyte*) res->data);

  return res;
}

void storage_set_item(void* cptr, char* key, bytes_t* content) {
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
  in3_cache_init(get_in3(env, ob));
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

  in3_ctx_t* ctx = ctx_new(get_in3(env, ob), (char*) str);

  if (!ctx->error) {
    res = in3_send_ctx(ctx);
    if (res >= 0) {
      d_token_t* r = d_get(ctx->responses[0], K_RESULT);
      if (r)
        result = d_create_json(r);
      else if ((r = d_get(ctx->responses[0], K_ERROR))) {
        if (d_type(r) == T_OBJECT) {
          str_range_t s = d_to_json(r);
          strncpy(error, s.data, s.len);
          error[s.len] = '\0';
        } else {
          strncpy(error, d_string(r), d_len(r));
          error[d_len(r)] = '\0';
        }
      } else if (ctx->error)
        strcpy(error, ctx->error);
      else
        strcpy(error, "No Result and also no error");

    } else if (ctx->error)
      strcpy(error, ctx->error);
    else
      strcpy(error, "Error sending the request");
  } else
    strcpy(error, ctx->error);

  //need to release this string when done with it in order to
  //avoid memory leak
  (*env)->ReleaseStringUTFChars(env, jreq, str);
  ctx_free(ctx);

  if (result) {
    js = (*env)->NewStringUTF(env, result);
    _free(result);
    return js;
  } else {
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

  in3_ctx_t* ctx = ctx_new(get_in3(env, ob), (char*) str);

  if (!ctx->error) {
    res = in3_send_ctx(ctx);
    if (res >= 0) {
      d_token_t* r = d_get(ctx->responses[0], K_RESULT);
      if (r)
        js = toObject(env, result = r);
      else if ((r = d_get(ctx->responses[0], K_ERROR))) {
        if (d_type(r) == T_OBJECT) {
          str_range_t s = d_to_json(r);
          strncpy(error, s.data, s.len);
          error[s.len] = '\0';
        } else {
          strncpy(error, d_string(r), d_len(r));
          error[d_len(r)] = '\0';
        }
      } else if (ctx->error)
        strcpy(error, ctx->error);
      else
        strcpy(error, "No Result and also no error");

    } else if (ctx->error)
      strcpy(error, ctx->error);
    else
      strcpy(error, "Error sending the request");
  } else
    strcpy(error, ctx->error);

  //need to release this string when done with it in order to
  //avoid memory leak
  (*env)->ReleaseStringUTFChars(env, jreq, str);

  ctx_free(ctx);

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
  if (in3->cache && in3->cache->cptr)
    (*env)->DeleteGlobalRef(env, (jobject) in3->cache->cptr);

  in3_free(in3);
}

in3_ret_t Java_in3_IN3_transport(in3_request_t* req) {
  uint64_t start = current_ms();
  //char** urls, int urls_len, char* payload, in3_response_t* res
  in3_ret_t success = IN3_OK;
  //payload
  size_t     payload_len = strlen(req->payload);
  jbyteArray jpayload    = (*jni)->NewByteArray(jni, payload_len);
  (*jni)->SetByteArrayRegion(jni, jpayload, 0, payload_len, (jbyte*) req->payload);

  // url-array
  jobject jurls = (*jni)->NewObjectArray(jni, req->urls_len, (*jni)->FindClass(jni, "java/lang/String"), NULL);
  for (int i = 0; i < req->urls_len; i++) (*jni)->SetObjectArrayElement(jni, jurls, i, (*jni)->NewStringUTF(jni, req->urls[i]));

  jclass       cls    = (*jni)->FindClass(jni, "in3/IN3");
  jmethodID    mid    = (*jni)->GetStaticMethodID(jni, cls, "sendRequest", "([Ljava/lang/String;[B)[[B");
  jobjectArray result = (*jni)->CallStaticObjectMethod(jni, cls, mid, jurls, jpayload);

  for (int i = 0; i < req->urls_len; i++) {
    jbyteArray content = result ? (*jni)->GetObjectArrayElement(jni, result, i) : NULL;
    if (content) {
      const size_t l     = (*jni)->GetArrayLength(jni, content);
      uint8_t*     bytes = _malloc(l);
      (*jni)->GetByteArrayRegion(jni, content, 0, l, (jbyte*) bytes);
      sb_add_range(&req->results[i].result, (char*) bytes, 0, l);
      _free(bytes);
    } else
      sb_add_chars(&req->results[i].error, "Could not fetch the data!");
    if (req->results[i].error.len) success = IN3_ERPC;
  }
  uint64_t end = current_ms();

  req->times = _malloc(sizeof(uint32_t) * req->urls_len);
  for (int i = 0; i < req->urls_len; i++) req->times[i] = (uint32_t)(end - start);

  return success;
}

/*
 * Class:     in3_eth1_TransactionRequest
 * Method:    abiEncode
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_eth1_TransactionRequest_abiEncode(JNIEnv* env, jclass clz, jstring fn, jstring json) {
  UNUSED_VAR(clz);
  const char*     fnc = (*env)->GetStringUTFChars(env, fn, 0);
  call_request_t* rq  = parseSignature((char*) fnc);
  (*env)->ReleaseStringUTFChars(env, fn, fnc);
  if (rq->error) {
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), rq->error);
    req_free(rq);
    return NULL;
  }

  const char* json_data = (*env)->GetStringUTFChars(env, json, 0);
  json_ctx_t* json_ctx  = parse_json((char*) json_data);
  if (!json_ctx) {
    req_free(rq);
    (*env)->ReleaseStringUTFChars(env, json, json_data);
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "Error parsing the data");
    return NULL;
  }

  if (set_data(rq, json_ctx->result, rq->in_data) < 0) {
    req_free(rq);
    json_free(json_ctx);
    (*env)->ReleaseStringUTFChars(env, json, json_data);
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "invalid data for the given signature");
    return NULL;
  }

  jstring res = (jstring) toObject(env, (d_token_t*) &rq->call_data->b);
  req_free(rq);
  json_free(json_ctx);
  (*env)->ReleaseStringUTFChars(env, json, json_data);
  return res;
}

/*
 * Class:     in3_eth1_TransactionRequest
 * Method:    abiDecode
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Lin3/JSON;
 */
JNIEXPORT jobject JNICALL Java_in3_eth1_TransactionRequest_abiDecode(JNIEnv* env, jclass clz, jstring fn, jstring data) {
  UNUSED_VAR(clz);
  const char*     fnc = (*env)->GetStringUTFChars(env, fn, 0);
  call_request_t* rq  = parseSignature((char*) fnc);
  (*env)->ReleaseStringUTFChars(env, fn, fnc);
  if (rq->error) {
    req_free(rq);
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), rq->error);
    return NULL;
  }

  const char* jdata = (*env)->GetStringUTFChars(env, data, 0);
  int         l     = strlen(jdata);
  uint8_t*    bdata = alloca(l >> 1);
  l                 = hex_to_bytes((char*) jdata + 2, l - 2, bdata, l);
  (*env)->ReleaseStringUTFChars(env, data, jdata);

  json_ctx_t* res    = req_parse_result(rq, bytes(bdata, l));
  jobject     result = res ? toObject(env, res->result) : NULL;
  req_free(rq);
  if (res)
    json_free(res);
  else
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Error"), "Error decoding the data");

  return result;
}

/*
 * Class:     in3_eth1_SimpleWallet
 * Method:    getAddressFromKey
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_eth1_SimpleWallet_getAddressFromKey(JNIEnv* env, jclass clz, jstring jkey) {
  UNUSED_VAR(clz);
  const char* key = (*env)->GetStringUTFChars(env, jkey, 0);

  bytes32_t prv_key;
  uint8_t   public_key[65], sdata[32];
  hex_to_bytes((char*) key, -1, prv_key, 32);
  bytes_t pubkey_bytes = {.data = public_key + 1, .len = 64};
  ecdsa_get_public_key65(&secp256k1, prv_key, public_key);
  sha3_to(&pubkey_bytes, sdata);
  (*env)->ReleaseStringUTFChars(env, jkey, key);
  char tmp[43];
  bytes_to_hex(sdata + 12, 20, tmp + 2);
  tmp[0] = '0';
  tmp[1] = 'x';
  return (*env)->NewStringUTF(env, tmp);
}

/*
 * Class:     in3_eth1_SimpleWallet
 * Method:    signData
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_eth1_SimpleWallet_signData(JNIEnv* env, jclass clz, jstring jkey, jstring jdata) {
  UNUSED_VAR(clz);
  const char* key    = (*env)->GetStringUTFChars(env, jkey, 0);
  const char* data   = (*env)->GetStringUTFChars(env, jdata, 0);
  int         data_l = strlen(data) / 2 - 1;
  uint8_t     key_bytes[32], *data_bytes = alloca(data_l + 1), dst[65];

  hex_to_bytes((char*) key + 2, -1, key_bytes, 32);
  data_l      = hex_to_bytes((char*) data + 2, -1, data_bytes, data_l + 1);
  jstring res = NULL;

  if (ecdsa_sign(&secp256k1, HASHER_SHA3K, key_bytes, data_bytes, data_l, dst, dst + 64, NULL) >= 0) {
    char tmp[133];
    bytes_to_hex(dst, 65, tmp + 2);
    tmp[0] = '0';
    tmp[1] = 'x';
    res    = (*env)->NewStringUTF(env, tmp);
  }
  (*env)->ReleaseStringUTFChars(env, jkey, key);
  (*env)->ReleaseStringUTFChars(env, jdata, data);
  return res;
}

/*
 * Class:     in3_eth1_SimpleWallet
 * Method:    decodeKeystore
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_eth1_SimpleWallet_decodeKeystore(JNIEnv* env, jclass clz, jstring json, jstring passphrase) {
  UNUSED_VAR(clz);
  UNUSED_VAR(env);
  UNUSED_VAR(json);
  UNUSED_VAR(passphrase);
  return NULL;
}

in3_ret_t jsign(void* pk, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst) {
  in3_ctx_t* ctx = (in3_ctx_t*) pk;
  UNUSED_VAR(type);
  jclass    cls    = (*jni)->GetObjectClass(jni, ctx->client->cache->cptr);
  jmethodID mid    = (*jni)->GetMethodID(jni, cls, "getSigner", "()Lin3/utils/Signer;");
  jobject   signer = (*jni)->CallObjectMethod(jni, ctx->client->cache->cptr, mid);

  if (!signer) return -1;

  char *data = alloca(message.len * 2 + 3), address[43];
  data[0] = address[0] = '0';
  data[1] = address[1] = 'x';
  bytes_to_hex(message.data, message.len, data + 2);
  bytes_to_hex(account.data, account.len, address + 2);

  jstring jdata      = (*jni)->NewStringUTF(jni, data);
  jstring jaddress   = (*jni)->NewStringUTF(jni, address);
  cls                = (*jni)->GetObjectClass(jni, signer);
  mid                = (*jni)->GetMethodID(jni, cls, "sign", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
  jstring jsignature = (*jni)->CallObjectMethod(jni, signer, mid, jdata, jaddress);

  if (!jsignature) return -2;
  const char* signature = (*jni)->GetStringUTFChars(jni, jsignature, 0);
  hex_to_bytes((char*) signature, -1, dst, 65);
  (*jni)->ReleaseStringUTFChars(jni, jsignature, signature);
  return 65;
}

void in3_set_jclient_config(in3_t* c, jobject jclient) {
  // Create a java configuration object
  // set the configuration object inside ob
  jclass  jconfigclass            = (*jni)->FindClass(jni, "in3/config/ClientConfiguration");
  jobject jclientconfigurationobj = (*jni)->NewObject(jni, jconfigclass, (*jni)->GetMethodID(jni, jconfigclass, "<init>", "()V"));

  jmethodID set_request_count_mid = (*jni)->GetMethodID(jni, jconfigclass, "setRequestCount", "(I)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_request_count_mid, (jint) c->request_count);

  jmethodID set_auto_update_list_mid = (*jni)->GetMethodID(jni, jconfigclass, "setAutoUpdateList", "(Z)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_auto_update_list_mid, (jboolean)(c->flags & FLAGS_AUTO_UPDATE_LIST) != 0);

  jclass    jproofcls     = (*jni)->FindClass(jni, "in3/Proof");
  jmethodID set_proof_mid = (*jni)->GetMethodID(jni, jconfigclass, "setProof", "(Lin3/Proof;)V");
  jfieldID  jproof        = (*jni)->GetStaticFieldID(jni, jproofcls, "full", "Lin3/Proof;");
  if (c->proof == PROOF_NONE) {
    jproof = (*jni)->GetStaticFieldID(jni, jproofcls, "none", "Lin3/Proof;");
  } else if (c->proof == PROOF_STANDARD) {
    jproof = (*jni)->GetStaticFieldID(jni, jproofcls, "standard", "Lin3/Proof;");
  }

  jobject proofval = (jobject)(*jni)->GetStaticObjectField(jni, jproofcls, jproof);
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_proof_mid, proofval);

  jmethodID set_max_attempts_mid = (*jni)->GetMethodID(jni, jconfigclass, "setMaxAttempts", "(I)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_max_attempts_mid, (jint) c->max_attempts);

  jmethodID set_signature_count_mid = (*jni)->GetMethodID(jni, jconfigclass, "setSignatureCount", "(I)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_signature_count_mid, (jint) c->signature_count);

  jmethodID set_finality_mid = (*jni)->GetMethodID(jni, jconfigclass, "setFinality", "(I)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_finality_mid, (jint) c->finality);

  jmethodID set_include_code_mid = (*jni)->GetMethodID(jni, jconfigclass, "setIncludeCode", "(Z)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_include_code_mid, (jboolean)(c->flags & FLAGS_INCLUDE_CODE) != 0);

  jmethodID set_keep_in3_mid = (*jni)->GetMethodID(jni, jconfigclass, "setKeepIn3", "(Z)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_keep_in3_mid, (jboolean)(c->flags & FLAGS_KEEP_IN3) != 0);

  jmethodID set_use_binary_mid = (*jni)->GetMethodID(jni, jconfigclass, "setUseBinary", "(Z)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_use_binary_mid, (jboolean)(c->flags & FLAGS_BINARY) != 0);

  jmethodID set_use_http_mid = (*jni)->GetMethodID(jni, jconfigclass, "setUseHttp", "(Z)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_use_http_mid, (jboolean)(c->flags & FLAGS_HTTP) != 0);

  jmethodID set_stats_mid = (*jni)->GetMethodID(jni, jconfigclass, "setStats", "(Z)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_stats_mid, (jboolean)(c->flags & FLAGS_STATS) != 0);

  jmethodID set_max_code_cache_mid = (*jni)->GetMethodID(jni, jconfigclass, "setMaxCodeCache", "(J)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_max_code_cache_mid, (jlong) c->max_code_cache);

  jmethodID set_timeout_mid = (*jni)->GetMethodID(jni, jconfigclass, "setTimeout", "(J)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_timeout_mid, (jlong) c->timeout);

  jmethodID set_min_deposit_mid = (*jni)->GetMethodID(jni, jconfigclass, "setMinDeposit", "(J)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_min_deposit_mid, (jlong) c->min_deposit);

  jmethodID set_node_props_mid = (*jni)->GetMethodID(jni, jconfigclass, "setNodeProps", "(J)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_node_props_mid, (jlong) c->request_count);

  jmethodID set_node_limit_mid = (*jni)->GetMethodID(jni, jconfigclass, "setNodeLimit", "(J)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_node_limit_mid, (jlong) c->node_limit);

  jmethodID set_replace_latest_block_mid = (*jni)->GetMethodID(jni, jconfigclass, "setReplaceLatestBlock", "(I)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_replace_latest_block_mid, (jint) c->replace_latest_block);

  jmethodID set_max_block_cache_mid = (*jni)->GetMethodID(jni, jconfigclass, "setMaxBlockCache", "(J)V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, set_max_block_cache_mid, (jlong) c->max_block_cache);

  for (int i = 0; i < c->chains_length; i++) {
    char        tmp[67]         = {'0', 'x'};
    in3_chain_t chain           = c->chains[i];
    jclass      jchainclass     = (*jni)->FindClass(jni, "in3/config/ChainConfiguration");
    jobject     jchainconfigobj = (*jni)->NewObject(jni, jchainclass, (*jni)->GetMethodID(jni, jchainclass, "<init>", "(JLin3/config/ClientConfiguration;)V"), (jlong) chain.chain_id, jclientconfigurationobj);

    jmethodID set_needs_update_mid = (*jni)->GetMethodID(jni, jchainclass, "setNeedsUpdate", "(Z)V");
    (*jni)->CallVoidMethod(jni, jchainconfigobj, set_needs_update_mid, (jboolean)(chain.nodelist_upd8_params != NULL));

    jmethodID set_contract_mid = (*jni)->GetMethodID(jni, jchainclass, "setContract", "(Ljava/lang/String;)V");
    bytes_to_hex(chain.contract->data, chain.contract->len, tmp + 2);
    (*jni)->CallVoidMethod(jni, jchainconfigobj, set_contract_mid, (*jni)->NewStringUTF(jni, tmp));

    jmethodID set_registry_id_mid = (*jni)->GetMethodID(jni, jchainclass, "setRegistryId", "(Ljava/lang/String;)V");
    bytes_to_hex(chain.registry_id, 32, tmp + 2);
    (*jni)->CallVoidMethod(jni, jchainconfigobj, set_registry_id_mid, (*jni)->NewStringUTF(jni, tmp));

    jclass jnodeconfigclass = (*jni)->FindClass(jni, "in3/config/NodeConfiguration");

    for (int i = 0; i < chain.nodelist_length; i++) {
      in3_node_t node            = chain.nodelist[i];
      jobject    jnodeconfigobj  = (*jni)->NewObject(jni, jnodeconfigclass, (*jni)->GetMethodID(jni, jnodeconfigclass, "<init>", "(Lin3/config/ChainConfiguration;)V"), jchainconfigobj);
      jmethodID  set_address_mid = (*jni)->GetMethodID(jni, jnodeconfigclass, "setAddress", "(Ljava/lang/String;)V");
      bytes_to_hex(node.address->data, node.address->len, tmp + 2);
      (*jni)->CallVoidMethod(jni, jnodeconfigobj, set_address_mid, (*jni)->NewStringUTF(jni, tmp));
      jmethodID set_url_mid = (*jni)->GetMethodID(jni, jnodeconfigclass, "setUrl", "(Ljava/lang/String;)V");
      (*jni)->CallVoidMethod(jni, jnodeconfigobj, set_url_mid, (*jni)->NewStringUTF(jni, node.url));
      jmethodID set_props_mid = (*jni)->GetMethodID(jni, jnodeconfigclass, "setProps", "(J)V");
      (*jni)->CallVoidMethod(jni, jnodeconfigobj, set_props_mid, (jlong) node.props);
    }

    if (chain.whitelist) {
      jmethodID set_white_list_contract_mid = (*jni)->GetMethodID(jni, jchainclass, "setWhiteListContract", "(Ljava/lang/String;)V");
      bytes_to_hex(chain.whitelist->contract, 20, tmp + 2);
      (*jni)->CallVoidMethod(jni, jchainconfigobj, set_white_list_contract_mid, (*jni)->NewStringUTF(jni, tmp));

      jobjectArray jwhitelist = (jobjectArray)(*jni)->NewObjectArray(
          jni,
          chain.whitelist->addresses.len / 20 + 1, // Check if this is the correct length
          (*jni)->FindClass(jni, "java/lang/String"),
          NULL);

      for (uint32_t j = 0; j < chain.whitelist->addresses.len; j += 20) {
        bytes_to_hex(chain.whitelist->addresses.data + j, 20, tmp + 2);
        (*jni)->SetObjectArrayElement(jni, jwhitelist, j / 20, (*jni)->NewStringUTF(jni, tmp));
      }

      jmethodID set_white_list_mid = (*jni)->GetMethodID(jni, jconfigclass, "setWhiteList", "([Ljava/lang/String;)V");
      (*jni)->CallVoidMethod(jni, jchainconfigobj, set_white_list_mid, jwhitelist);
    }
  }

  jclass    jclientclass   = (*jni)->GetObjectClass(jni, jclient);
  jmethodID set_config_mid = (*jni)->GetMethodID(jni, jclientclass, "setConfig", "(Lin3/config/ClientConfiguration;)V");
  (*jni)->CallVoidMethod(jni, jclient, set_config_mid, jclientconfigurationobj);

  jmethodID marked_as_synced = (*jni)->GetMethodID(jni, jconfigclass, "markAsSynced", "()V");
  (*jni)->CallVoidMethod(jni, jclientconfigurationobj, marked_as_synced);
}

#ifdef IPFS
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

/*
 * Class:     in3_IN3
 * Method:    init
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_in3_IN3_init(JNIEnv* env, jobject ob, jlong jchain) {
  in3_t* in3 = in3_for_chain(jchain);
  in3_register_eth_api();
  in3_log_set_level(LOG_DEBUG);
  in3->transport          = Java_in3_IN3_transport;
  in3->cache              = _malloc(sizeof(in3_storage_handler_t));
  in3->cache->cptr        = (*env)->NewGlobalRef(env, ob);
  in3->cache->get_item    = storage_get_item;
  in3->cache->set_item    = storage_set_item;
  in3->cache->clear       = storage_clear;
  in3->signer             = _malloc(sizeof(in3_signer_t));
  in3->signer->sign       = jsign;
  in3->signer->prepare_tx = NULL;
  in3->signer->wallet     = in3->cache->cptr;
  jni                     = env;

  in3_set_jclient_config(in3, ob);

  return (jlong)(size_t) in3;
}

JNIEXPORT jstring JNICALL Java_in3_IN3_getVersion(JNIEnv* env, jclass c) {
  UNUSED_VAR(c);
  return (*env)->NewStringUTF(env, IN3_VERSION);
}
