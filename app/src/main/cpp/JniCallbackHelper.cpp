//
// Created by EDZ on 2020/1/9.
//

#include "JniCallbackHelper.h"
#include "macro.h"

JniCallbackHelper::JniCallbackHelper(JavaVM *javaVm, JNIEnv *env, jobject object) {
    this->javaVm = javaVm;
    this->env = env;
    // 一旦涉及跨方法、跨线程，需object创建全局引用
//    this->object = object;
    this->object = env->NewGlobalRef(object);
    jclass clazz = env->GetObjectClass(this->object);
    jmd_prepared = env->GetMethodID(clazz, "prepared", "()V"); // javap 获取方法签名
    jmd_onError = env->GetMethodID(clazz, "onError", "(I)V");
}

JniCallbackHelper::~JniCallbackHelper() {
    javaVm = 0;
    env->DeleteGlobalRef(object);
    object = 0;
    env = 0;
}

void JniCallbackHelper::onPrepared(int thread_mode) {
    if(thread_mode == THREAD_MAIN){
        // env不支持跨线程
        env->CallVoidMethod(object, jmd_prepared);
    }else{
        // 子线程处理
        JNIEnv *env1;
        javaVm->AttachCurrentThread(&env1, 0);
        env1->CallVoidMethod(object, jmd_prepared);
        javaVm->DetachCurrentThread();
    }
}

void JniCallbackHelper::onError(int thread_mode, int error_code) {
    if (thread_mode == THREAD_MAIN) {
        //主线程
        env->CallVoidMethod(object, jmd_onError);
    } else {
        //子线程
        JNIEnv *env_child;
        javaVm->AttachCurrentThread(&env_child, 0);
        env_child->CallVoidMethod(object, jmd_onError, error_code);
        javaVm->DetachCurrentThread();
    }
}
