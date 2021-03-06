//
// Created by EDZ on 2020/1/9.
//

#ifndef MYPLAYER_BASECHANNEL_H
#define MYPLAYER_BASECHANNEL_H

extern "C" {
#include <libavcodec/avcodec.h>
};

#include "safe_queue.h"

class BaseChannel {
public:
    BaseChannel(int streamIndex, AVCodecContext *codecContext, AVRational time_base)
            : stream_index(streamIndex),
              codecContext(codecContext),
              time_base(time_base){
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel() {
        packets.clear();
    }

    /**
     * 释放AVPacket *
     * @param packet
     */
    static void releaseAVPacket(AVPacket **packet) {
        if (*packet) {
            av_packet_free(packet);
            *packet = 0;
        }
    }

    /**
     * 释放 AVFrame *
     * @param packet
     */
    static void releaseAVFrame(AVFrame **frame) {
        if (*frame) {
            av_frame_free(frame);
            *frame = 0;
        }
    }
    int isPlaying;
    int stream_index;
    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    AVCodecContext *codecContext = 0;
    AVRational time_base;
    double audio_time;
};

#endif //MYPLAYER_BASECHANNEL_H
