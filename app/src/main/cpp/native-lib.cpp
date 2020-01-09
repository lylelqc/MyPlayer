#include <jni.h>
#include <string>
#include "MyCPlayer.h"
#include "JniCallbackHelper.h"
#include <android/native_window_jni.h>
#include <pthread.h>

extern "C" {
#include <libavutil/avutil.h>
}

JavaVM *javaVm = 0;
MyCPlayer *player = 0;
ANativeWindow *window = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // 静态初始化

jint JNI_OnLoad(JavaVM *vm, void *args){
    javaVm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_test_myplayer_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

void renderFrame(uint8_t *src_data, int width, int height, int src_linesize){
    pthread_mutex_lock(&lock);
    if(!window){
        pthread_mutex_unlock(&lock);
        return;
    }
    // 设置窗口属性
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;
    // 锁失败
    if(ANativeWindow_lock(window, &windowBuffer, 0)){
        ANativeWindow_release(window);
        window = 0;
        return;
    }
    // 填充rgba数据给dst_data（缓冲区目标）
    uint8_t  *dst_data = static_cast<uint8_t *>(windowBuffer.bits);
    int dst_linesize = windowBuffer.stride * 4;
    for (int i = 0; i < windowBuffer.height; ++i) {
        // 逐行拷贝
        memcpy(dst_data + i * dst_linesize, src_data + i * src_linesize, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&lock);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_test_myplayer_MyPlayer_prepareNative(JNIEnv *env, jobject thiz, jstring data_source_) {
    const char *data_source = env->GetStringUTFChars(data_source_, 0);
    JniCallbackHelper *helper = new JniCallbackHelper(javaVm, env, thiz);
    player = new MyCPlayer(data_source, helper);
    player->setRenderCallback(renderFrame);
    player->prepare();
    env->ReleaseStringUTFChars(data_source_, data_source);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_test_myplayer_MyPlayer_startNative(JNIEnv *env, jobject thiz) {
    if(player){
        player->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_test_myplayer_MyPlayer_stopNative(JNIEnv *env, jobject thiz) {
    // TODO: implement stopNative()
}

extern "C"
JNIEXPORT void JNICALL
Java_com_test_myplayer_MyPlayer_releaseNative(JNIEnv *env, jobject thiz) {
    // TODO: implement releaseNative()
}

extern "C"
JNIEXPORT void JNICALL
Java_com_test_myplayer_MyPlayer_setSurfaceNative(JNIEnv *env, jobject thiz, jobject surface) {
    pthread_mutex_lock(&lock);
    // 释放之前的显示窗口
    if(window){
        ANativeWindow_release(window);
        window = 0;
    }
    // 创建新的窗口用于视频的显示
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&lock);
}