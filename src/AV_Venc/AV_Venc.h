#pragma once
/*
    编码
*/

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavutil/log.h"
}

class AV_Venc
{
public:
    AV_Venc();
    ~AV_Venc();

    /**
     * @brief open encoder
     * @param[1]:video width
     * @param[2]:video height
     * @param[3]:encoder name
     * @param[4]:need frmae to transfrom -- if use this param, please use (AVPacket* Encoder(AVPacket *pkt))
    */
    bool Open(int width, int height, const char *codec, bool needFrmae=false);

    void Close();

    /**
     * @brief start encoder
     * @param[1]:encoding frame data
     * @return: Encoded data
    */
    AVPacket* Encoder(AVFrame *pFrame);

    /**
     * @brief start encoder
     * @param[1]:encoding packet data
     * @return: Encoded data
    */
    AVPacket* Encoder(AVPacket *pkt);

    AVFrame* Yuyv422Pkt2Yuv420P(AVPacket *pkt);

private:
    /**
     * @brief create frame and packet
     * @param[1]:video width
     * @param[2]:video height
     * @param[2]:create frame
     * @return
    */
    bool OnFramePacket(int width, int height, bool needframe=false);

private:
    AVCodecContext *m_pCodecCtx;
    AVFrame* m_pFrame;
    AVPacket* m_packet;
};
