//
// Created by EDZ on 2020/1/9.
//

#include "VideoChannel.h"

void dropAVPacket(queue<AVPacket *> &q){
    while(!q.empty()){
        AVPacket *packet = q.front();
        // AV_PKT_FLAG_KEY 关键帧
        if(packet->flags != AV_PKT_FLAG_KEY){
            // 非关键帧才丢
            BaseChannel::releaseAVPacket(&packet);
            q.pop();
        }else{
            break;
        }
    }
}

void dropAVFrame(queue<AVFrame *> &q){
    // 未解码才考虑关键帧
    while(!q.empty()){
        // 取一个出来
        AVFrame *frame = q.front();
        BaseChannel::releaseAVFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int stream_index, AVCodecContext *pContext, AVRational time_base, int fps)
        : BaseChannel(
        stream_index, pContext, time_base) {
    this->fps = fps;
    packets.setSyncCallback(dropAVPacket);
    frames.setSyncCallback(dropAVFrame);
}

void *task_video_decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_decode();

    return 0;// 一定要return！
}

/**
 *  AVPacket消费队列里面的包
 *  消费速度比生成速度慢
 */
void VideoChannel::video_decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        /**
         * 泄漏点2: 控制AVFrame队列
         */
        if(isPlaying && frames.size() > 100){
            //休眠
            av_usleep(10 * 1000); //microsecond 微秒
            continue;
        }
        //从队列中取 视频压缩数据包 AVPacket
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
        // AVFrame 生产
        frames.push(frame);
    }//end while
    releaseAVPacket(&packet);
}

void *task_video_play(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->video_play();

    return 0;// 一定要return！
}

/**
 * AVFrame 消费
 */
void VideoChannel::video_play() {
    AVFrame *frame = 0;
    uint8_t *dst_data[4];
    int dst_linesize[4];

    SwsContext *sws_ctx = sws_getContext(codecContext->width, codecContext->height,
                                         codecContext->pix_fmt,
                                         codecContext->width, codecContext->height, AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR, NULL, NULL, NULL);
    av_image_alloc(dst_data, dst_linesize,
                   codecContext->width, codecContext->height, AV_PIX_FMT_RGBA, 1);
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            //如果停止播放了，跳出循环
            break;
        }
        if (!ret) {
            continue;
        }
        //格式转换 yuv > rgba
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, codecContext->height, dst_data,
                  dst_linesize
        );

        /**
         * 音视频同步
         */
        // 控制延时时间
        double extra_delay = frame->repeat_pict / (2 * fps); // 每一帧的额外延时时间
        double avg_delay = 1.0 / fps; // 根据fps得到的平均延时时间
        double real_delay = extra_delay + avg_delay;
//        av_usleep(real_delay * 1000000); 多休眠了!!!!

        // 视频时间
        double video_time = frame->best_effort_timestamp * av_q2d(time_base);

        // 类似gif 哑剧 无音频
        if(!audio_channel){
            av_usleep(real_delay * 1000000);
        }else{
            // 以音频的时间为基准
            double audio_time = audio_channel->audio_time;
            double time_diff = video_time - audio_time;
            if(time_diff > 0){
                // 比音频快， 等音频
                // 拖动进度条 time_diff很大
                if(time_diff > 1){
                    av_usleep((real_delay * 2) * 1000000); // 慢慢等
                }else{
                    av_usleep((real_delay + time_diff) * 1000000);
                }
            }else if(time_diff < 0){
                // 画面延迟
                // 比音频慢， 追音频（丢帧）
                if(fabs(time_diff) >= 0.05){
//                packets.sync();
                    frames.sync();
                    continue;
                }
            }else{
                // 完美同步,基本不可能
            }

        }
        // TODO

        //dst_data : rgba格式的图像数据
        //宽+高+linesize
        //渲染（绘制）
        renderCallback(dst_data[0], codecContext->width, codecContext->height, dst_linesize[0]);

        releaseAVFrame(&frame);
    }
    isPlaying = 0;
    releaseAVFrame(&frame);
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
}

/**
 * 1、解码
 * 2、播放
 */
void VideoChannel::start() {
    isPlaying = 1;
    packets.setWork(1);
    frames.setWork(1);
    pthread_create(&pid_video_decode, 0, task_video_decode, this);
    pthread_create(&pid_video_play, 0, task_video_play, this);
}

// 设置渲染回调
void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audio_channel = audioChannel;
}
