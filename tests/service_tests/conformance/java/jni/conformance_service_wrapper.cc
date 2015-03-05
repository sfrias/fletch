// Copyright (c) 2015, the Fletch project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE.md file.

// Generated file. Do not edit.

#include <jni.h>
#include <stdlib.h>

#include "service_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ANDROID
  typedef JNIEnv* AttachEnvType;
#else
  typedef void* AttachEnvType;
#endif

static ServiceId service_id_ = kNoServiceId;

JNIEXPORT void JNICALL Java_fletch_ConformanceService_Setup(JNIEnv*, jclass) {
  service_id_ = ServiceApiLookup("ConformanceService");
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_TearDown(JNIEnv*, jclass) {
  ServiceApiTerminate(service_id_);
}

static JNIEnv* attachCurrentThreadAndGetEnv(JavaVM* vm) {
  AttachEnvType result = NULL;
  if (vm->AttachCurrentThread(&result, NULL) != JNI_OK) {
    // TODO(ager): Nicer error recovery?
    exit(1);
  }
  return reinterpret_cast<JNIEnv*>(result);
}

static void detachCurrentThread(JavaVM* vm) {
  if (vm->DetachCurrentThread() != JNI_OK) {
    // TODO(ager): Nicer error recovery?
    exit(1);
  }
}

static jobject createByteArray(JNIEnv* env, char* memory, int size) {
  jbyteArray result = env->NewByteArray(size);
  jbyte* contents = reinterpret_cast<jbyte*>(memory);
  env->SetByteArrayRegion(result, 0, size, contents);
  free(memory);
  return result;
}

static jobject createByteArrayArray(JNIEnv* env, char* memory, int size) {
  jobjectArray array = env->NewObjectArray(size, env->FindClass("[B"), NULL);
  for (int i = 0; i < size; i++) {
    int64_t address = *reinterpret_cast<int64_t*>(memory + 8 + (i * 16));
    int size = *reinterpret_cast<int*>(memory + 16 + (i * 16));
    char* contents = reinterpret_cast<char*>(address);
    env->SetObjectArrayElement(array, i, createByteArray(env, contents, size));
  }
  free(memory);
  return array;
}

static jobject getRootSegment(JNIEnv* env, char* memory) {
  int32_t segments = *reinterpret_cast<int32_t*>(memory);
  if (segments == 0) {
    int32_t size = *reinterpret_cast<int32_t*>(memory + 4);
    return createByteArray(env, memory, size);
  }
  return createByteArrayArray(env, memory, segments);
}

class CallbackInfo {
 public:
  CallbackInfo(jobject jcallback, JavaVM* jvm)
      : callback(jcallback), vm(jvm) { }
  jobject callback;
  JavaVM* vm;
};

static int computeMessage(JNIEnv* env,
                          jobject builder,
                          char** buffer,
                          jobject callback,
                          JavaVM* vm) {
  jclass clazz = env->GetObjectClass(builder);
  jmethodID methodId = env->GetMethodID(clazz, "isSegmented", "()Z");
  jboolean isSegmented =  env->CallBooleanMethod(builder, methodId);

  CallbackInfo* info = NULL;
  if (callback != NULL) {
    info = new CallbackInfo(callback, vm);
  }

  if (isSegmented) {
    methodId = env->GetMethodID(clazz, "getSegments", "()[[B");
    jobjectArray array = (jobjectArray)env->CallObjectMethod(builder, methodId);
    int segments = env->GetArrayLength(array);

    int size = 56 + (segments * 16);
    *buffer = reinterpret_cast<char*>(malloc(size));
    int offset = 56;
    for (int i = 0; i < segments; i++) {
      jbyteArray segment = (jbyteArray)env->GetObjectArrayElement(array, i);
      int segment_length = env->GetArrayLength(segment);
      jboolean is_copy;
      jbyte* data = env->GetByteArrayElements(segment, &is_copy);
      // TODO(ager): Release this again.
      *reinterpret_cast<void**>(*buffer + offset) = data;
      // TODO(ager): Correct sizing.
      *reinterpret_cast<int*>(*buffer + offset + 8) = segment_length;
      offset += 16;
    }

    // Mark the request as being segmented.
    *reinterpret_cast<int32_t*>(*buffer + 40) = segments;
    // Set the callback information.
    *reinterpret_cast<CallbackInfo**>(*buffer + 32) = info;
    return size;
  }

  methodId = env->GetMethodID(clazz, "getSingleSegment", "()[B");
  jbyteArray segment = (jbyteArray)env->CallObjectMethod(builder, methodId);
  int segment_length = env->GetArrayLength(segment);
  jboolean is_copy;
  // TODO(ager): Release this again.
  jbyte* data = env->GetByteArrayElements(segment, &is_copy);
  *buffer = reinterpret_cast<char*>(data);
  // Mark the request as being non-segmented.
  *reinterpret_cast<int64_t*>(*buffer + 40) = 0;
  // Set the callback information.
  *reinterpret_cast<CallbackInfo**>(*buffer + 32) = info;
  // TODO(ager): Correct sizing.
  return segment_length;
}

static const MethodId _kgetAgeId = reinterpret_cast<MethodId>(1);

JNIEXPORT jint JNICALL Java_fletch_ConformanceService_getAge(JNIEnv* _env, jclass, jobject person) {
  char* buffer = NULL;
  int size = computeMessage(_env, person, &buffer, NULL, NULL);
  ServiceApiInvoke(service_id_, _kgetAgeId, buffer, size);
  return *reinterpret_cast<int64_t*>(buffer + 48);
}

static void Unwrap_int32_24(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "(I)V");
  env->CallVoidMethod(info->callback, methodId, result);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_getAgeAsync(JNIEnv* _env, jclass, jobject person, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  char* buffer = NULL;
  int size = computeMessage(_env, person, &buffer, callback, vm);
  ServiceApiInvokeAsync(service_id_, _kgetAgeId, Unwrap_int32_24, buffer, size);
}

static const MethodId _kgetBoxedAgeId = reinterpret_cast<MethodId>(2);

JNIEXPORT jint JNICALL Java_fletch_ConformanceService_getBoxedAge(JNIEnv* _env, jclass, jobject box) {
  char* buffer = NULL;
  int size = computeMessage(_env, box, &buffer, NULL, NULL);
  ServiceApiInvoke(service_id_, _kgetBoxedAgeId, buffer, size);
  return *reinterpret_cast<int64_t*>(buffer + 48);
}

static void Unwrap_int32_8(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "(I)V");
  env->CallVoidMethod(info->callback, methodId, result);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_getBoxedAgeAsync(JNIEnv* _env, jclass, jobject box, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  char* buffer = NULL;
  int size = computeMessage(_env, box, &buffer, callback, vm);
  ServiceApiInvokeAsync(service_id_, _kgetBoxedAgeId, Unwrap_int32_8, buffer, size);
}

static const MethodId _kgetAgeStatsId = reinterpret_cast<MethodId>(3);

JNIEXPORT jobject JNICALL Java_fletch_ConformanceService_getAgeStats(JNIEnv* _env, jclass, jobject person) {
  char* buffer = NULL;
  int size = computeMessage(_env, person, &buffer, NULL, NULL);
  ServiceApiInvoke(service_id_, _kgetAgeStatsId, buffer, size);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(_env, memory);
  jclass resultClass = _env->FindClass("fletch/AgeStats");
  jmethodID create = _env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/AgeStats;");
  jobject resultObject = _env->CallStaticObjectMethod(resultClass, create, rootSegment);
  return resultObject;
}

static void Unwrap_AgeStats_24(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(env, memory);
  jclass resultClass = env->FindClass("fletch/AgeStats");
  jmethodID create = env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/AgeStats;");
  jobject resultObject = env->CallStaticObjectMethod(resultClass, create, rootSegment);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "(Lfletch/AgeStats;)V");
  env->CallVoidMethod(info->callback, methodId, resultObject);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_getAgeStatsAsync(JNIEnv* _env, jclass, jobject person, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  char* buffer = NULL;
  int size = computeMessage(_env, person, &buffer, callback, vm);
  ServiceApiInvokeAsync(service_id_, _kgetAgeStatsId, Unwrap_AgeStats_24, buffer, size);
}

static const MethodId _kcreateAgeStatsId = reinterpret_cast<MethodId>(4);

JNIEXPORT jobject JNICALL Java_fletch_ConformanceService_createAgeStats(JNIEnv* _env, jclass, jint averageAge, jint sum) {
  static const int kSize = 56;
  char _bits[kSize];
  char* _buffer = _bits;
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  *reinterpret_cast<jint*>(_buffer + 48) = averageAge;
  *reinterpret_cast<jint*>(_buffer + 52) = sum;
  ServiceApiInvoke(service_id_, _kcreateAgeStatsId, _buffer, kSize);
  int64_t result = *reinterpret_cast<int64_t*>(_buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(_env, memory);
  jclass resultClass = _env->FindClass("fletch/AgeStats");
  jmethodID create = _env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/AgeStats;");
  jobject resultObject = _env->CallStaticObjectMethod(resultClass, create, rootSegment);
  return resultObject;
}

static void Unwrap_AgeStats_8(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(env, memory);
  jclass resultClass = env->FindClass("fletch/AgeStats");
  jmethodID create = env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/AgeStats;");
  jobject resultObject = env->CallStaticObjectMethod(resultClass, create, rootSegment);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "(Lfletch/AgeStats;)V");
  env->CallVoidMethod(info->callback, methodId, resultObject);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_createAgeStatsAsync(JNIEnv* _env, jclass, jint averageAge, jint sum, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  static const int kSize = 56 + 1 * sizeof(void*);
  char* _buffer = reinterpret_cast<char*>(malloc(kSize));
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  *reinterpret_cast<jint*>(_buffer + 48) = averageAge;
  *reinterpret_cast<jint*>(_buffer + 52) = sum;
  CallbackInfo* info = new CallbackInfo(callback, vm);
  *reinterpret_cast<CallbackInfo**>(_buffer + 32) = info;
  ServiceApiInvokeAsync(service_id_, _kcreateAgeStatsId, Unwrap_AgeStats_8, _buffer, kSize);
}

static const MethodId _kcreatePersonId = reinterpret_cast<MethodId>(5);

JNIEXPORT jobject JNICALL Java_fletch_ConformanceService_createPerson(JNIEnv* _env, jclass, jint children) {
  static const int kSize = 56;
  char _bits[kSize];
  char* _buffer = _bits;
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  *reinterpret_cast<jint*>(_buffer + 48) = children;
  ServiceApiInvoke(service_id_, _kcreatePersonId, _buffer, kSize);
  int64_t result = *reinterpret_cast<int64_t*>(_buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(_env, memory);
  jclass resultClass = _env->FindClass("fletch/Person");
  jmethodID create = _env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/Person;");
  jobject resultObject = _env->CallStaticObjectMethod(resultClass, create, rootSegment);
  return resultObject;
}

static void Unwrap_Person_8(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(env, memory);
  jclass resultClass = env->FindClass("fletch/Person");
  jmethodID create = env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/Person;");
  jobject resultObject = env->CallStaticObjectMethod(resultClass, create, rootSegment);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "(Lfletch/Person;)V");
  env->CallVoidMethod(info->callback, methodId, resultObject);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_createPersonAsync(JNIEnv* _env, jclass, jint children, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  static const int kSize = 56 + 1 * sizeof(void*);
  char* _buffer = reinterpret_cast<char*>(malloc(kSize));
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  *reinterpret_cast<jint*>(_buffer + 48) = children;
  CallbackInfo* info = new CallbackInfo(callback, vm);
  *reinterpret_cast<CallbackInfo**>(_buffer + 32) = info;
  ServiceApiInvokeAsync(service_id_, _kcreatePersonId, Unwrap_Person_8, _buffer, kSize);
}

static const MethodId _kcreateNodeId = reinterpret_cast<MethodId>(6);

JNIEXPORT jobject JNICALL Java_fletch_ConformanceService_createNode(JNIEnv* _env, jclass, jint depth) {
  static const int kSize = 56;
  char _bits[kSize];
  char* _buffer = _bits;
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  *reinterpret_cast<jint*>(_buffer + 48) = depth;
  ServiceApiInvoke(service_id_, _kcreateNodeId, _buffer, kSize);
  int64_t result = *reinterpret_cast<int64_t*>(_buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(_env, memory);
  jclass resultClass = _env->FindClass("fletch/Node");
  jmethodID create = _env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/Node;");
  jobject resultObject = _env->CallStaticObjectMethod(resultClass, create, rootSegment);
  return resultObject;
}

static void Unwrap_Node_8(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  char* memory = reinterpret_cast<char*>(result);
  jobject rootSegment = getRootSegment(env, memory);
  jclass resultClass = env->FindClass("fletch/Node");
  jmethodID create = env->GetStaticMethodID(resultClass, "create", "(Ljava/lang/Object;)Lfletch/Node;");
  jobject resultObject = env->CallStaticObjectMethod(resultClass, create, rootSegment);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "(Lfletch/Node;)V");
  env->CallVoidMethod(info->callback, methodId, resultObject);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_createNodeAsync(JNIEnv* _env, jclass, jint depth, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  static const int kSize = 56 + 1 * sizeof(void*);
  char* _buffer = reinterpret_cast<char*>(malloc(kSize));
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  *reinterpret_cast<jint*>(_buffer + 48) = depth;
  CallbackInfo* info = new CallbackInfo(callback, vm);
  *reinterpret_cast<CallbackInfo**>(_buffer + 32) = info;
  ServiceApiInvokeAsync(service_id_, _kcreateNodeId, Unwrap_Node_8, _buffer, kSize);
}

static const MethodId _kcountId = reinterpret_cast<MethodId>(7);

JNIEXPORT jint JNICALL Java_fletch_ConformanceService_count(JNIEnv* _env, jclass, jobject person) {
  char* buffer = NULL;
  int size = computeMessage(_env, person, &buffer, NULL, NULL);
  ServiceApiInvoke(service_id_, _kcountId, buffer, size);
  return *reinterpret_cast<int64_t*>(buffer + 48);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_countAsync(JNIEnv* _env, jclass, jobject person, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  char* buffer = NULL;
  int size = computeMessage(_env, person, &buffer, callback, vm);
  ServiceApiInvokeAsync(service_id_, _kcountId, Unwrap_int32_24, buffer, size);
}

static const MethodId _kdepthId = reinterpret_cast<MethodId>(8);

JNIEXPORT jint JNICALL Java_fletch_ConformanceService_depth(JNIEnv* _env, jclass, jobject node) {
  char* buffer = NULL;
  int size = computeMessage(_env, node, &buffer, NULL, NULL);
  ServiceApiInvoke(service_id_, _kdepthId, buffer, size);
  return *reinterpret_cast<int64_t*>(buffer + 48);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_depthAsync(JNIEnv* _env, jclass, jobject node, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  char* buffer = NULL;
  int size = computeMessage(_env, node, &buffer, callback, vm);
  ServiceApiInvokeAsync(service_id_, _kdepthId, Unwrap_int32_24, buffer, size);
}

static const MethodId _kfooId = reinterpret_cast<MethodId>(9);

JNIEXPORT void JNICALL Java_fletch_ConformanceService_foo(JNIEnv* _env, jclass) {
  static const int kSize = 56;
  char _bits[kSize];
  char* _buffer = _bits;
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  ServiceApiInvoke(service_id_, _kfooId, _buffer, kSize);
}

static void Unwrap_void_8(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "()V");
  env->CallVoidMethod(info->callback, methodId);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_fooAsync(JNIEnv* _env, jclass, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  static const int kSize = 56 + 1 * sizeof(void*);
  char* _buffer = reinterpret_cast<char*>(malloc(kSize));
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  CallbackInfo* info = new CallbackInfo(callback, vm);
  *reinterpret_cast<CallbackInfo**>(_buffer + 32) = info;
  ServiceApiInvokeAsync(service_id_, _kfooId, Unwrap_void_8, _buffer, kSize);
}

static const MethodId _kbarId = reinterpret_cast<MethodId>(10);

JNIEXPORT jint JNICALL Java_fletch_ConformanceService_bar(JNIEnv* _env, jclass, jobject empty) {
  char* buffer = NULL;
  int size = computeMessage(_env, empty, &buffer, NULL, NULL);
  ServiceApiInvoke(service_id_, _kbarId, buffer, size);
  return *reinterpret_cast<int64_t*>(buffer + 48);
}

static void Unwrap_int32_0(void* raw) {
  char* buffer = reinterpret_cast<char*>(raw);
  CallbackInfo* info = *reinterpret_cast<CallbackInfo**>(buffer + 32);
  JNIEnv* env = attachCurrentThreadAndGetEnv(info->vm);
  int64_t result = *reinterpret_cast<int64_t*>(buffer + 48);
  jclass clazz = env->GetObjectClass(info->callback);
  jmethodID methodId = env->GetMethodID(clazz, "handle", "(I)V");
  env->CallVoidMethod(info->callback, methodId, result);
  env->DeleteGlobalRef(info->callback);
  detachCurrentThread(info->vm);
  delete info;
  free(buffer);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_barAsync(JNIEnv* _env, jclass, jobject empty, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  char* buffer = NULL;
  int size = computeMessage(_env, empty, &buffer, callback, vm);
  ServiceApiInvokeAsync(service_id_, _kbarId, Unwrap_int32_0, buffer, size);
}

static const MethodId _kpingId = reinterpret_cast<MethodId>(11);

JNIEXPORT jint JNICALL Java_fletch_ConformanceService_ping(JNIEnv* _env, jclass) {
  static const int kSize = 56;
  char _bits[kSize];
  char* _buffer = _bits;
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  ServiceApiInvoke(service_id_, _kpingId, _buffer, kSize);
  return *reinterpret_cast<int64_t*>(_buffer + 48);
}

JNIEXPORT void JNICALL Java_fletch_ConformanceService_pingAsync(JNIEnv* _env, jclass, jobject _callback) {
  jobject callback = _env->NewGlobalRef(_callback);
  JavaVM* vm;
  _env->GetJavaVM(&vm);
  static const int kSize = 56 + 1 * sizeof(void*);
  char* _buffer = reinterpret_cast<char*>(malloc(kSize));
  *reinterpret_cast<int64_t*>(_buffer + 40) = 0;
  CallbackInfo* info = new CallbackInfo(callback, vm);
  *reinterpret_cast<CallbackInfo**>(_buffer + 32) = info;
  ServiceApiInvokeAsync(service_id_, _kpingId, Unwrap_int32_8, _buffer, kSize);
}

#ifdef __cplusplus
}
#endif