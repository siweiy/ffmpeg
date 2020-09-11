#pragma once

#include "AV_Header.h"
extern "C"
{
#include <string.h>
#include "libavutil/frame.h"
#include "libavutil/log.h"
}

class AV_ImgFmtConver
{
public:
    AV_ImgFmtConver *GetInstall();
    void Release();

    /*
        根据 AVFrame 转换格式数据直接返回
    */
    static uint8_t* AVFrame2YUV(AVFrame *pFrame);

    /*
        已知格式，直接调用转换
    */
    static uint8_t *AVFrame2YUV420(AVFrame *pFrame);
    static uint8_t* AVFrame2YUV420P(AVFrame *pFrame);
    static uint8_t* AVFrame2YUV422P(AVFrame *pFrame);
    static uint8_t* AVFrame2YUV422(AVFrame *pFrame);
    static uint8_t* AVFrame2RGB24(AVFrame *pFrame);
    static uint8_t* YUV420P2RGB24(const uint8_t *yuv_buffer_in, int width, int height);

private:
    AV_ImgFmtConver();
    ~AV_ImgFmtConver();

private:
    static AV_ImgFmtConver *m_ImgFmtConver;
};
