//
// Created by EDZ on 2020/1/9.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int stream_index, AVCodecContext *codecContext, AVRational time_base) : BaseChannel(
        stream_index, codecContext, time_base) {
    out_sample_rate = 44100;
    // 16bits = 2字节
    // 输出样本的大小
    out_sample_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    // 输出声道数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    // 输出缓冲区大小
    out_buffers_size = out_sample_rate *out_sample_size * out_channels;
    // 初始化输出缓冲区
    out_buffers = static_cast<uint8_t *>(malloc(out_buffers_size));
    memset(out_buffers, 0, out_buffers_size);
    // 重采样上下文。 申请空间并设置参数
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, out_sample_rate,
            codecContext->channel_layout, codecContext->sample_fmt, codecContext->sample_rate,
            0, 0);
    //初始化上下文
    swr_init(swrContext);
}

void *task_audio_decode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audio_decode();

    return 0;// 一定要return！
}

void AudioChannel::audio_decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        /**
         * 泄漏点2: 控制AVFrame队列
         */
        if(isPlaying && frames.size() > 100){
            //休眠
            av_usleep(10*1000); //microsecond 微秒
            continue;
        }
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

void *task_audio_play(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->audio_play();

    return 0;// 一定要return！
}

//4.3 创建回调函数 / 一直调用
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *args) {
    // 声音处理
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    int pcm_size = audioChannel->getPCM();
    if(pcm_size > 0){
        (*bq)->Enqueue(bq, audioChannel->out_buffers, pcm_size);
    }
}

/**
 * 获取 pcm 数据
 * @return  数据大小
 */
int AudioChannel::getPCM() {
    int pcm_size = 0;
    AVFrame *frame = 0;

    // 方便流程控制
    while (isPlaying) {
        int ret = frames.pop(frame); // 声音格式不清楚
        if (!isPlaying) {
            //如果停止播放了，跳出循环
            break;
        }
        if (!ret) {
            continue;
        }
        // pcm 处理
        // 重采样
        // 上下文， 输出缓冲区（大小根据数据声音类型来定）
        // a * b / c
        // swr_get_delay 截取剩余的个数
        int out_nb_sample = av_rescale_rnd(
                swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples,
                out_sample_rate, frame->sample_rate, AV_ROUND_UP);
        // 每个声道的样本数
        int sample_per_channel = swr_convert(swrContext, &out_buffers, out_nb_sample,
                (const uint8_t **)(frame->data), frame->nb_samples);

        // 重采样后pcm数据大小 = 每个声道的样本数 * 声道数 * 一个样本的大小
        pcm_size = sample_per_channel * out_sample_size * out_channels;

        // 把音频的时间告诉视频 才能同步
        // 每帧音频的时间
        audio_time = frame->best_effort_timestamp * av_q2d(time_base);
        break;
    }
//    isPlaying = 0; 不工作了
    releaseAVFrame(&frame);
    return pcm_size;
}

/**
 * 播放pcm原始数据
 */
void AudioChannel::audio_play() {

    SLresult result;
    // 1.1 创建引擎对象：SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.2 初始化引擎
    // 解引用
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.3 获取引擎接口 SLEngineItf engineInterface
    // 解引用
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 设置混音器
     */
    // 2.1 创建混音器：SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.2 初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //不启用混响可以不用获取混音器接口
    // 获得混音器接口
    //result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
    //                                         &outputMixEnvironmentalReverb);
    //if (SL_RESULT_SUCCESS == result) {
    //设置混响 ： 默认。
    //SL_I3DL2_ENVIRONMENT_PRESET_ROOM: 室内
    //SL_I3DL2_ENVIRONMENT_PRESET_AUDITORIUM : 礼堂 等
    //const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    //(*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
    //       outputMixEnvironmentalReverb, &settings);
    //}

    /**
     * 创建播放器
     */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                       2};
    //pcm数据格式
    //SL_DATAFORMAT_PCM：数据格式为pcm格式
    //2：双声道
    //SL_SAMPLINGRATE_44_1：采样率为44100 兼容性最广
    //SL_PCMSAMPLEFORMAT_FIXED_16：采样格式为16bit
    //SL_PCMSAMPLEFORMAT_FIXED_16：数据大小为16bit
    //SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT：左右声道（双声道）
    //SL_BYTEORDER_LITTLEENDIAN：小端模式
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                                   SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    //3.2 配置音轨（输出）
    //设置混音器
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};
    //需要的接口 操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3 创建播放器
    result = (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &audioSrc, &audioSnk, 1, ids, req);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //3.4 初始化播放器：SLObjectItf bqPlayerObject
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //3.5 获取播放器接口：SLPlayItf bqPlayerPlay
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    //4.1 获取播放器队列接口：SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);

    //4.2 设置回调 void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    // 5.设置播放器状态为播放状态
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);

    // 6.手动激活回调函数
    bqPlayerCallback(bqPlayerBufferQueue, this);
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

AudioChannel::~AudioChannel() {
    // 7、释放
    //7.1 设置停止状态
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    //7.2 销毁播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueue = 0;
    }
    //7.3 销毁混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }
    //7.4 销毁引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineInterface = 0;
    }
}
