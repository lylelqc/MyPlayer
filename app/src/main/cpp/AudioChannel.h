//
// Created by EDZ on 2020/1/9.
//

#ifndef MYPLAYER_AUDIOCHANNEL_H
#define MYPLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

class AudioChannel : public BaseChannel {

public:
    AudioChannel(int stream_index, AVCodecContext *codecContext);

    //引擎
    SLObjectItf engineObject = 0;
    //引擎接口
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerPlay = 0;
    //播放器队列接口
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = 0;

    void start();

    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;

    void audio_decode();

    void audio_play();
};


#endif //MYPLAYER_AUDIOCHANNEL_H
