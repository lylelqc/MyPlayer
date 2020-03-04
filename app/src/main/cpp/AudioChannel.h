//
// Created by EDZ on 2020/1/9.
//

#ifndef MYPLAYER_AUDIOCHANNEL_H
#define MYPLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C"{
#include <libswresample/swresample.h>
#include <libavutil/time.h>
};

class AudioChannel : public BaseChannel {

public:
    AudioChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base);

    virtual ~AudioChannel();

    void start();

    void audio_decode();

    void audio_play();

    int getPCM();

    int out_sample_rate;
    int out_buffers_size;
    int out_sample_size;
    int out_channels;
    uint8_t *out_buffers;
private:

    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;
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
    SwrContext *swrContext;
};


#endif //MYPLAYER_AUDIOCHANNEL_H
