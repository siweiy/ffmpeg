#pragma once

/*
    解码
*/

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavutil/log.h"
}

class AV_Vdec
{
public:
    AV_Vdec();
    /*
        参数2表示需要解码的流index，视频、音频等要分开，创建多几个类处理即可
    */
    AV_Vdec(AVFormatContext *fmt_ctx, int streamIndex);
    ~AV_Vdec();

    int Open();
    int Close();
    AVFrame *Decoder(AVPacket *pkt);
    /*
        解码输入文件时使用，在最后解码完成后while循环调用，直到数据为空
    */
    AVFrame *DecoderEnd(AVPacket *pkt);

    void setFmtCtxAndIndex(AVFormatContext *fmt_ctx, int streamIndex);
    AVCodecContext *getAVCodecContext();

private:
    int VdecInitMember();

private:
    AVFormatContext *m_pFormatCtx; // 格式上下文
    AVCodecContext *m_pCodecCtx;   // 解码器上下文
    AVCodec *m_pCodec;             // 解码器指针
    AVFrame *m_pFrame;             // 解码后的数据
    int m_streamIndex;             // 流index
};
