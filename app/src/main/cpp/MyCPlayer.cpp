//
// Created by EDZ on 2020/1/9.
//

#include "MyCPlayer.h"

//中转站

MyCPlayer::MyCPlayer(const char *data_source, JniCallbackHelper *helper) {
//    this->data_source = data_source; //悬空指针问题

    // 申请内存
    // 尾部有\0的问题
    this->data_source = new char[strlen(data_source) + 1];
    // 拷贝
    strcpy(this->data_source, data_source);
    this->jni_callback_helper = helper;
}

MyCPlayer::~MyCPlayer() {
    if(data_source){
        delete data_source;
        data_source = 0;
    }
}

/**
 * 任务线程
 * @param args
 * @return
 */
void *task_prepare(void *args){
    MyCPlayer *player = static_cast<MyCPlayer *>(args);
    player->_prepare();

    return 0; // 一定要return
}

void MyCPlayer::prepare(){
    /**
     * 开子线程
     */
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

void MyCPlayer::_prepare() {
    // 1 打开媒体地址
    formatContext = avformat_alloc_context();
    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary, "timeout", "5000000", 0);
    /**
     * 1, 上下文
     * 2，url: 文件路径或直播地址
     * 3，AVInputFormat ：输入的封装格式
     * 4，参数
     */
    int ret = avformat_open_input(&formatContext, data_source, 0, &dictionary);
    if(ret){
        if (jni_callback_helper) {
            jni_callback_helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    // 2 查找流信息
    ret = avformat_find_stream_info(formatContext, 0);
    if(ret < 0){
        if (jni_callback_helper) {
            jni_callback_helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    // 3 根据流信息个数循环查找
    for(int i= 0; i < formatContext->nb_streams; i++){
        // 4 获取媒体流
        AVStream *stream = formatContext->streams[i];

        // 5 从流中获取解码这段流的参数
        AVCodecParameters *codecParameters = stream->codecpar;

        // 6 通过流的编解码id来读取当前流的解码器
        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if(!codec){
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }

        // 7 创建解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if(!codecContext){
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        // 8 设置解码器上下文参数
        ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if(ret < 0){
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        // 9.打开解码器
        ret = avcodec_open2(codecContext, codec, 0);
        if(ret){
            if (jni_callback_helper) {
                jni_callback_helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        // 10.从编码器参数中获取流类型
        if(codecParameters->codec_type == AVMEDIA_TYPE_AUDIO){
            // 音频流
            audio_channel = new AudioChannel(i, codecContext);
        }else if(codecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
            // 视频流
            video_channel = new VideoChannel(i, codecContext);
            video_channel->setRenderCallback(renderCallback);
        }
    }

    // 11.如果没有音视频流
    if(!audio_channel && !video_channel){
        if (jni_callback_helper) {
            jni_callback_helper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    // 准备工作over,可以播放了（回调）
    if(jni_callback_helper){
        jni_callback_helper->onPrepared(THREAD_CHILD);
    }
}

/**
 * 任务线程
 * @param args
 * @return
 */
void *task_start(void *args){
    MyCPlayer *player = static_cast<MyCPlayer *>(args);
    player->_start();

    return 0; // 一定要return
}

/**
 * 开始播放（先解码后播放）
 */
void MyCPlayer::start() {
    isPlaying = 1;
    if(video_channel){
        video_channel->start();
    }
    if(audio_channel){
        audio_channel->start();
    }
    pthread_create(&pid_start, 0, task_start, this);
}

/**
 * 真正开始播放
 * 读取 音视频包 加入相应的音/视频队列
 *
 */
void MyCPlayer::_start() {
    while(isPlaying){
        AVPacket *packet = av_packet_alloc();
        int ret = av_read_frame(formatContext, packet);
        if(!ret){
            // read success 判断数据包类型是音频还是视频， 根据流索引来判断
            if(video_channel && video_channel->stream_index == packet->stream_index){
                // 视频数据包
                video_channel->packets.push(packet);
            }else if(audio_channel && audio_channel->stream_index == packet->stream_index){
                // 音频数据包
                audio_channel->packets.push(packet);

            }
        }else if(ret == AVERROR_EOF){
            // end of file, 读完了,是否播放完
        }else{
            break;
        }

    }
    isPlaying = 0;
//    videoChannel->stop();
}

void MyCPlayer::setRenderCallback(RenderCallback callback) {
    this->renderCallback = callback;
}
