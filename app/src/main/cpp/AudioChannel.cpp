//
// Created by EDZ on 2020/1/9.
//

#include "AudioChannel.h"
#include <libavformat/avformat.h>

AudioChannel::AudioChannel(int stream_index, AVCodecContext *codecContext) : BaseChannel(stream_index, codecContext) {

}

void *task_audio_decode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audio_decode();

    return 0;// 一定要return！
}

void *task_audio_play(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audio_play();

    return 0;// 一定要return！
}

void AudioChannel::audio_decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        //从队列中取视频压缩数据包 AVPacket
        int ret = packets.pop(packet);
        if (!isPlaying) {
            //如果停止播放了，跳出循环
            break;
        }
        if (!ret) {
            continue;
        }
        //把数据包发给解码器进行解码
        ret = avcodec_send_packet(codecContext, packet);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        //发送一个数据包成功
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }
        //成功解码一个数据包，得到解码后的数据包 AVFrame, 加入队列
        frames.push(frame);
    }//end while
    releaseAVPacket(&packet);
}

/**
 * 播放pcm原始数据
 */
void AudioChannel::audio_play() {
    AVFrame *frame = 0;

    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            //如果停止播放了，跳出循环
            break;
        }
        if (!ret) {
            continue;
        }
        // pcm 处理


        releaseAVFrame(&frame);
    }
    isPlaying = 0;
    releaseAVFrame(&frame);
}

/**
 * 解码
 * 播放
 */
void AudioChannel::start() {
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_audio_decode, 0, task_audio_decode, this);
    pthread_create(&pid_audio_play, 0, task_audio_play, this);
}
