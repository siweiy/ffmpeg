#pragma once

/*
    格式转换
*/

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

class AV_FrameConvert
{
public:
    // 默认，宽高参数记得设置
    AV_FrameConvert();
    // 初始化宽高
    AV_FrameConvert(int in_width, int in_height);
    // 传入解码器，使用 Open(enum AVPixelFormat sws_fmt, int sws_algorithn);
    AV_FrameConvert(AVCodecContext *codec_ctx);
    ~AV_FrameConvert();

    /** 
     *  @brief
        @param out_fmt: convertion format
        @param sws_algorithn: convertion algorithn
        @param av_codec: option
    */
    bool Open(enum AVPixelFormat out_fmt, int sws_algorithn, AVCodecContext *av_codec=NULL);

    /** 
     *  @brief
        @param in_fmt   : input format
        @param out_fmt  : output format
        @param sws_algorithn: convertion algorithn
        @param av_codec: option
    */
    bool Open(enum AVPixelFormat in_fmt, enum AVPixelFormat out_fmt, int sws_algorithn, AVCodecContext *av_codec=NULL);

    /** 
     *  @brief
     *  @param int_width: input width
     *  @param in_height: input height
     *  @param int_fmt  : input format
     *  @param out_fmt  : output format
     *  @param sws_algorithn: convertion algorithn
     *  @param av_codec: option
    */
    bool Open(int in_width, int in_height, enum AVPixelFormat in_fmt, enum AVPixelFormat out_fmt, int sws_algorithn, AVCodecContext *av_codec=NULL);

    void Close();

    /** 
     *  @brief Convert data
     *  @param pFrame: Source image data
    */
    uint8_t *transform(AVFrame *pFrame);

    int BufferSize();

    void setCodecContext(AVCodecContext *pCodecCtx);

private:

    /**
    * @brief
    * @param int_fmt  : input format
    * @param out_fmt  : output format
    * @param sws_algorithn: convertion algorithn
    */
    bool Init(enum AVPixelFormat in_fmt, enum AVPixelFormat out_fmt, int sws_algorithn);

private:
    struct SwsContext *m_imgConvertCtx; // 图像处理上下文
    AVCodecContext *m_pCodecCtx;
    AVFrame *m_pFrameSws;
    int m_outBufferSize;
    uint8_t *m_outBuffer;
    int m_width;
    int m_height;
};