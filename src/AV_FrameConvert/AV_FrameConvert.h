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
    AV_FrameConvert(AVCodecContext *codec_ctx);
    ~AV_FrameConvert();

    /*
        @param sws_fmt: convertion format
        @param sws_algorithn: convertion algorithn
    */
    bool Open(enum AVPixelFormat sws_fmt, int sws_algorithn);
    void Close();

    /*  Convert data
        @param pFrame: Source image data
    */
    uint8_t *transform(AVFrame *pFrame);
    int getBufferSize();

private:
    struct SwsContext *m_imgConvertCtx; // 图像处理上下文
    AVCodecContext *m_pCodecCtx;
    AVFrame *m_pFrameSws;
    int m_outBufferSize;
    uint8_t *m_outBuffer;
};