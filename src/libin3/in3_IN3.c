#include "in3_IN3.h"
#include <client/client.h>
#include <client/context.h>
#include <client/keys.h>
#include <client/send.h>
#include <eth_full.h>
#include <in3_curl.h>
#include <util/mem.h>

static in3_t* get_in3(JNIEnv* env, jobject obj) {
  jlong l = (*env)->GetLongField(env, obj, (*env)->GetFieldID(env, (*env)->GetObjectClass(env, obj), "ptr", "J"));
  return (in3_t*) (size_t) l;
}

/*
 * Class:     in3_IN3
 * Method:    getCacheTimeout
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getCacheTimeout(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->cacheTimeout;
}

/*
 * Class:     in3_IN3
 * Method:    setCacheTimeout
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setCacheTimeout(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->cacheTimeout = val;
}

/*
 * Class:     in3_IN3
 * Method:    getNodeLimit
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getNodeLimit(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->nodeLimit;
}

/*
 * Class:     in3_IN3
 * Method:    setNodeLimit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setNodeLimit(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->nodeLimit = val;
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
  in3->key       = _malloc(sizeof(bytes_t));
  in3->key->len  = (*env)->GetArrayLength(env, val);
  in3->key->data = _malloc(in3->key->len);
  (*env)->GetByteArrayRegion(env, val, 0, in3->key->len, (jbyte*) in3->key->data);
}

/*
 * Class:     in3_IN3
 * Method:    getMaxCodeCache
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getMaxCodeCache(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->maxCodeCache;
}

/*
 * Class:     in3_IN3
 * Method:    setMaxCodeCache
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setMaxCodeCache(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->maxCodeCache = val;
}

/*
 * Class:     in3_IN3
 * Method:    getMaxBlockCache
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getMaxBlockCache(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->maxBlockCache;
}

/*
 * Class:     in3_IN3
 * Method:    setMaxBlockCache
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setMaxBlockCache(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->maxBlockCache = val;
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
  return get_in3(env, ob)->requestCount;
}
/*
 * Class:     in3_IN3
 * Method:    setRequestCount
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setRequestCount(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->requestCount = val;
}

/*
 * Class:     in3_IN3
 * Method:    getSignatureCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getSignatureCount(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->signatureCount;
}

/*
 * Class:     in3_IN3
 * Method:    setSignatureCount
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setSignatureCount(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->signatureCount = val;
}

/*
 * Class:     in3_IN3
 * Method:    getMinDeposit
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_in3_IN3_getMinDeposit(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->minDeposit;
}

/*
 * Class:     in3_IN3
 * Method:    setMinDeposit
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setMinDeposit(JNIEnv* env, jobject ob, jlong val) {
  get_in3(env, ob)->minDeposit = val;
}

/*
 * Class:     in3_IN3
 * Method:    getReplaceLatestBlock
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_in3_IN3_getReplaceLatestBlock(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->replaceLatestBlock;
}

/*
 * Class:     in3_IN3
 * Method:    getReplaceLatestBlock
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setReplaceLatestBlock(JNIEnv* env, jobject ob, jint val) {
  get_in3(env, ob)->replaceLatestBlock = val;
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
  return get_in3(env, ob)->chainId;
}

/*
 * Class:     in3_IN3
 * Method:    setChainId
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setChainId(JNIEnv* env, jobject ob, jlong val) {
  get_in3(env, ob)->chainId = val;
}

/*
 * Class:     in3_IN3
 * Method:    isAutoUpdateList
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_in3_IN3_isAutoUpdateList(JNIEnv* env, jobject ob) {
  return get_in3(env, ob)->autoUpdateList;
}

/*
 * Class:     in3_IN3
 * Method:    setAutoUpdateList
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setAutoUpdateList(JNIEnv* env, jobject ob, jboolean val) {
  get_in3(env, ob)->autoUpdateList = val;
}

/*
 * Class:     in3_IN3
 * Method:    getStorageProvider
 * Signature: ()Lin3/StorageProvider;
 */
JNIEXPORT jobject JNICALL Java_in3_IN3_getStorageProvider(JNIEnv* env, jobject ob) {
  in3_t* in3 = get_in3(env, ob);
  if (in3->cacheStorage && in3->cacheStorage->cptr)
    return (jobject) in3->cacheStorage->cptr;
  return NULL;
}

static JNIEnv* jni = NULL;

bytes_t* storage_get_item(void* cptr, char* key) {
  jobject   handler = (jobject) cptr;
  jclass    cls     = (*jni)->GetObjectClass(jni, handler);
  jmethodID mid     = (*jni)->GetMethodID(jni, cls, "getItem", "(Ljava/lang/String;)[B");
  jstring   js      = (*jni)->NewStringUTF(jni, key);

  jbyteArray result = (jbyteArray)(*jni)->CallObjectMethod(jni, handler, mid, js);
  if (result == NULL) return NULL;

  bytes_t* res = _malloc(sizeof(bytes_t));
  res->len     = (*jni)->GetArrayLength(jni, result);
  res->data    = _malloc(res->len);
  (*jni)->GetByteArrayRegion(jni, result, 0, res->len, (jbyte*) res->data);

  return res;
}

void storage_set_item(void* cptr, char* key, bytes_t* content) {
  jobject    handler = (jobject) cptr;
  jclass     cls     = (*jni)->GetObjectClass(jni, handler);
  jmethodID  mid     = (*jni)->GetMethodID(jni, cls, "setItem", "(Ljava/lang/String;[B)V");
  jstring    js      = (*jni)->NewStringUTF(jni, key);
  jbyteArray bytes   = (*jni)->NewByteArray(jni, content->len);
  (*jni)->SetByteArrayRegion(jni, bytes, 0, content->len, (jbyte*) content->data);
  (*jni)->CallVoidMethod(jni, handler, mid, js, bytes);
}

/*
 * Class:     in3_IN3
 * Method:    setStorageProvider
 * Signature: (Lin3/StorageProvider;)V
 */
JNIEXPORT void JNICALL Java_in3_IN3_setStorageProvider(JNIEnv* env, jobject ob, jobject provider) {
  in3_t* in3 = get_in3(env, ob);
  if (in3->cacheStorage) _free(in3->cacheStorage);
  in3->cacheStorage           = _malloc(sizeof(in3_storage_handler_t));
  in3->cacheStorage->cptr     = provider;
  in3->cacheStorage->get_item = storage_get_item;
  in3->cacheStorage->set_item = storage_set_item;
}

/*
 * Class:     in3_IN3
 * Method:    send
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_in3_IN3_send(JNIEnv* env, jobject ob, jstring jreq) {
  jni = env;

  const char* str    = (*env)->GetStringUTFChars(env, jreq, 0);
  char*       result = NULL;
  char        error[10000];
  int         res;
  jstring     js = NULL;

  in3_ctx_t* ctx = new_ctx(get_in3(env, ob), (char*) str);

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
  free_ctx(ctx);

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

/*
 * Class:     in3_IN3
 * Method:    free
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_in3_IN3_free(JNIEnv* env, jobject ob) {
  in3_free(get_in3(env, ob));
}
/*
 * Class:     in3_IN3
 * Method:    init
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_in3_IN3_init(JNIEnv* env, jobject ob) {
  UNUSED_VAR(env);
  UNUSED_VAR(ob);
  in3_t* in3 = in3_new();
  in3_register_eth_full();
  in3->transport = send_curl;

  return (jlong)(size_t) in3;
}
