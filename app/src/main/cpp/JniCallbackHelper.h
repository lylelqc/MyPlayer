//
// Created by EDZ on 2020/1/9.
//

#ifndef MYPLAYER_JNICALLBACKHELPER_H
#define MYPLAYER_JNICALLBACKHELPER_H


#include <jni.h>

class JniCallbackHelper {

public:
    JniCallbackHelper(JavaVM *javaVm, JNIEnv *env, jobject object);

    ~JniCallbackHelper();

    void onPrepared(int thread_mode);
    void onError(int thread_mode, int error_code);

    JNIEnv *env = 0;
    jobject object;
    jmethodID jmd_prepared;
    jmethodID jmd_onError;
    JavaVM *javaVm;

};


#endif //MYPLAYER_JNICALLBACKHELPER_H
