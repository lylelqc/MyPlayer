package com.test.myplayer;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MyPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("native-lib");
    }

    //准备过程错误码
    public static final int ERROR_CODE_FFMPEG_PREPARE = 1000;
    //播放过程错误码
    public static final int ERROR_CODE_FFMPEG_PLAY = 2000;

    //打不开视频
    public static final int FFMPEG_CAN_NOT_OPEN_URL = (ERROR_CODE_FFMPEG_PREPARE - 1);

    //找不到媒体流信息
    public static final int FFMPEG_CAN_NOT_FIND_STREAMS = (ERROR_CODE_FFMPEG_PREPARE - 2);

    //找不到解码器
    public static final int FFMPEG_FIND_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 3);

    //无法根据解码器创建上下文
    public static final int FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 4);

    //根据流信息 配置上下文参数失败
    public static final int FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 5);

    //打开解码器失败
    public static final int FFMPEG_OPEN_DECODER_FAIL = (ERROR_CODE_FFMPEG_PREPARE - 6);

    //没有音视频
    public static final int FFMPEG_NOMEDIA = (ERROR_CODE_FFMPEG_PREPARE - 7);

    //读取媒体数据包失败
    public static final int FFMPEG_READ_PACKETS_FAIL = (ERROR_CODE_FFMPEG_PLAY - 1);

    private String dataSource;
    private SurfaceHolder surfaceHolder;
    private OnPrepareListener listener;
    private OnErrorListener onErrorListener;

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    /**
     * 准备工作
     */
    public void prepare(){
        prepareNative(dataSource);
    }

    public void start(){
        startNative();
    }

    public void stop(){
        stopNative();
    }

    public void release(){
        releaseNative();
    }

    /**
     * jni那边调用的
     */
    public void prepared(){
        if(null != listener){
            listener.onPrepared();
        }
    }

    public void setOnPrepareListener(OnPrepareListener listener) {
        this.listener = listener;
    }

    interface OnPrepareListener{
        void onPrepared();
    }

    /**
     * 给jni反射调用的
     */
    public void onError(int errorCode) {
        if (null != this.onErrorListener) {
            this.onErrorListener.onError(errorCode);
        }
    }

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    interface OnErrorListener {
        void onError(int errorCode);
    }

    /**
     * native 方法
     * @param dataSource
     */
    private native void prepareNative(String dataSource);
    private native void startNative();
    private native void stopNative();
    private native void releaseNative();
    private native void setSurfaceNative(Surface surface);

    public void setSurfaceView(SurfaceView surfaceView) {
        if(null != surfaceHolder){
            surfaceHolder.removeCallback(this);
        }
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        setSurfaceNative(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
