//
// Created by EDZ on 2020/1/9.
//

#ifndef MYPLAYER_MYCPLAYER_H
#define MYPLAYER_MYCPLAYER_H

#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JniCallbackHelper.h"
#include <cstring>
#include <pthread.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

#include "AudioChannel.h"
#include "VideoChannel.h"
#include "macro.h"


class MyCPlayer {
public:
    MyCPlayer(const char *data_source, JniCallbackHelper *pHelper);

    ~MyCPlayer();

    void prepare();

    void _prepare();

    void start();

    void _start();

    void setRenderCallback(RenderCallback renderCallback);

private:
    char *data_source;
    AudioChannel *audio_channel = 0;
    VideoChannel *video_channel = 0;
    JniCallbackHelper *jni_callback_helper = 0;
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *formatContext = 0;
    bool isPlaying;

    RenderCallback renderCallback;
};


#endif //MYPLAYER_MYCPLAYER_H
