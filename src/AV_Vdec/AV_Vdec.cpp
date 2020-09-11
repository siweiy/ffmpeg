#include "AV_Vdec.h"
#include <unistd.h>

AV_Vdec::AV_Vdec() : m_pFormatCtx(NULL),
                     m_streamIndex(-1),
                     m_pCodecCtx(NULL),
                     m_pCodec(NULL),
                     m_pFrame(NULL)
{
}

AV_Vdec::AV_Vdec(AVFormatContext *fmt_ctx, int streamIndex) : m_pFormatCtx(fmt_ctx),
                                                              m_streamIndex(streamIndex),
                                                              m_pCodecCtx(NULL),
                                                              m_pCodec(NULL),
                                                              m_pFrame(NULL)
{
}

AV_Vdec::~AV_Vdec()
{
    if (m_pFrame)
        av_frame_free(&m_pFrame);
    if (m_pCodecCtx)
        avcodec_close(m_pCodecCtx);
}

int AV_Vdec::VdecInitMember()
{
    m_pFrame = av_frame_alloc();
    if (!m_pFrame)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]alloc frame error: pFrame\n", __func__, __LINE__);
        return -1;
    }

    // 编解码器上下文
    // m_pCodecCtx = m_pFormatCtx->streams[m_streamIndex]->codec;// 已被抛弃
    m_pCodecCtx = avcodec_alloc_context3(m_pCodec);
    if (!m_pCodecCtx)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_alloc_context3 error\n", __func__, __LINE__);
        return -2;
    }
    if (avcodec_parameters_to_context(m_pCodecCtx, m_pFormatCtx->streams[m_streamIndex]->codecpar) < 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_parameters_to_context\n", __func__, __LINE__);
        return -3;
    }

    // m_pCodecCtx->bit_rate = 0;      //初始化为0
    // m_pCodecCtx->time_base.num = 1; //下面两行：一秒钟25帧
    // m_pCodecCtx->time_base.den = 10;
    m_pCodecCtx->frame_number = 1; //每包一个视频帧

    // 根据编解码器id寻找编解码器
    m_pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
    // m_pCodec = avcodec_find_decoder_by_name("h264");
    if (!m_pCodec)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_find_decoder error\n", __func__, __LINE__);
        return -4;
    }

    return 0;
}

void AV_Vdec::setFmtCtxAndIndex(AVFormatContext *fmt_ctx, int streamIndex)
{
    m_pFormatCtx = fmt_ctx;
    m_streamIndex = streamIndex;
}

int AV_Vdec::Open()
{
    int ret = 0;

    if (!m_pFormatCtx)
        return -1;

    if ((ret = VdecInitMember()) < 0)
    {
        return ret;
    }

    if (avcodec_open2(m_pCodecCtx, m_pCodec, NULL) < 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_open2 error\n", __func__, __LINE__);
        return -2;
    }
    return 0;
}

int AV_Vdec::Close()
{
    if (m_pFrame)
        av_frame_free(&m_pFrame);
    if (m_pCodecCtx)
        avcodec_close(m_pCodecCtx);
}

/*  将 AVPacket 解码为 AVFrame
    m_pFrame->data[0]存储Y
    m_pFrame->data[1]存储U
    m_pFrame->data[2]存储V
    而他们相对应的大小为：
    m_pFrame->linesize[0]为Y的大小
    m_pFrame->linesize[1]为U的大小
    m_pFrame->linesize[2]为V的大小
*/
AVFrame *AV_Vdec::Decoder(AVPacket *pkt)
{
    int got_picture = 1;
    // 判断视频流
    if (m_streamIndex == pkt->stream_index)
    {

#if 0
        // 解码, 将packet的数据解码到frame --- 此接口已被最新ffmpeg抛弃
        if (avcodec_decode_video2(m_pCodecCtx, m_pFrame, &got_picture, pkt) < 0)
        {
            av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_decode_video2 error\n", __func__, __LINE__);
            return NULL;
        }

        // 解码成功
        if (got_picture)
        {
            return m_pFrame;
        }

        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_decode_video2 error\n", __func__, __LINE__);

#else

        if (avcodec_send_packet(m_pCodecCtx, pkt) == 0)
        {
            got_picture = avcodec_receive_frame(m_pCodecCtx, m_pFrame);
            if (got_picture == 0)
            {
                return m_pFrame;
            }
            else
            {
                av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_receive_frame error\n", __func__, __LINE__);
            }
        }

#endif
    }

    return NULL;
}

AVFrame *AV_Vdec::DecoderEnd(AVPacket *pkt)
{
    int got_picture = 1;
    // 判断视频流
    if (m_streamIndex == pkt->stream_index)
    {
        // 处理最后的数据
        if (!(m_pCodec->capabilities & AV_CODEC_CAP_DELAY))
            goto __EXIT;

        if (avcodec_send_packet(m_pCodecCtx, pkt) == 0) // 当前packet已被free，即传入的是空
        {
            got_picture = avcodec_receive_frame(m_pCodecCtx, m_pFrame); // 解码直到无数据返回
            if (got_picture == 0)
            {
                return m_pFrame;
            }
            av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avcodec_receive_frame error\n", __func__, __LINE__);
        }
    }
__EXIT:

    return NULL;
}

AVCodecContext *AV_Vdec::getAVCodecContext()
{
    if (m_pCodecCtx)
        return m_pCodecCtx;

    return NULL;
}