/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class ru_iv_support_dll_Library */

#ifndef _Included_ru_iv_support_dll_Library
#define _Included_ru_iv_support_dll_Library
#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void JNICALL Java_ru_iv_support_dll_Library_register
  (JNIEnv *, jclass, jobject);

JNIEXPORT void JNICALL Java_ru_iv_support_dll_Library_send
  (JNIEnv *, jclass, jobject, jstring, jboolean);

JNIEXPORT jboolean JNICALL Java_ru_iv_support_dll_Library_hasDevice
  (JNIEnv *, jclass, jobject);

JNIEXPORT jboolean JNICALL Java_ru_iv_support_dll_Library_init
  (JNIEnv *, jclass);

JNIEXPORT jboolean JNICALL Java_ru_iv_support_dll_Library_destroy
(JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif