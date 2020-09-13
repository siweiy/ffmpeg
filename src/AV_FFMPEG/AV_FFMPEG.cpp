#include "AV_FFMPEG.h"

AV_FFMPEG::AV_FFMPEG() : m_pFormatCtx(NULL), m_packet(NULL)
{
    this->AVInit();
}

void AV_FFMPEG::AVInit()
{
    av_log_set_level(AV_LOG_INFO);
    // av_register_all();       // 新版本不需要注册
    // avcodec_register_all();  // 新版本不需要注册
    avformat_network_init();
    avdevice_register_all();
}

int AV_FFMPEG::AVMemberInit()
{
    m_audioIndex = -1;
    m_videoIndex = -1;
    m_subtitleIndex = -1;
    m_pFormatCtx = avformat_alloc_context(); // 申请av格式上下文空间
    if (!m_pFormatCtx)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]alloc context error\n", __func__, __LINE__);
        return -1;
    }
    m_packet = av_packet_alloc(); // 申请packet空间
    if (!m_packet)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]alloc packet error\n", __func__, __LINE__);
        return -2;
    }

    // m_pFormatCtx->probesize = 10000000; // 设置大小，防止内存不够出错
}

AV_FFMPEG::~AV_FFMPEG()
{
    if (m_pFormatCtx)
        avformat_free_context(m_pFormatCtx);
    avformat_network_deinit();
}

int AV_FFMPEG::Open(const char *rtsp_url, bool rtsp)
{
    AVMemberInit();

    AVDictionary *options = NULL;
    if (rtsp)
    {
        av_dict_set(&options, "buffer_size", "4096000", 0);
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
        av_dict_set(&options, "stimeout", "5000000", 0);
        av_dict_set(&options, "max_delay", "500000", 0);
        // av_dict_set(&options, "framerate", "25", 0);
        av_dict_set(&options, "fflags", "nobuffer", 0);
    }

    if (avformat_open_input(&m_pFormatCtx, rtsp_url, NULL, &options) < 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avformat_open_input rtsp or file error\n", __func__, __LINE__);
        return -1;
    }

    //注意：最后一个参数填0，打印输入流；最后一个参数填1，打印输出流
    av_dump_format(m_pFormatCtx, 0, rtsp_url, 0);

    if (findStreamIndex() < 0)
        return -2;

    return 0;
}

int AV_FFMPEG::Open(const char *dev, const char *video_fmt)
{
    AVMemberInit();

    AVDictionary *options = NULL;
    AVInputFormat *ifmt = av_find_input_format(video_fmt);

    if (avformat_open_input(&m_pFormatCtx, dev, ifmt, &options) < 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avformat_open_input dev error\n", __func__, __LINE__);
        return -1;
    }

    //注意：最后一个参数填0，打印输入流；最后一个参数填1，打印输出流
    av_dump_format(m_pFormatCtx, 0, dev, 0);

    if (findStreamIndex() < 0)
        return -2;

    return 0;
}

int AV_FFMPEG::findStreamIndex()
{
    if (avformat_find_stream_info(m_pFormatCtx, NULL) < 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avformat_open_input error\n", __func__, __LINE__);
        return -1;
    }

    m_audioIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (m_audioIndex >= 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]av_find_best_stream audio index:%d\n", __func__, __LINE__, m_audioIndex);
    }

    m_subtitleIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0);
    if (m_subtitleIndex >= 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]av_find_best_stream subtitle index:%d\n", __func__, __LINE__, m_subtitleIndex);
    }

    m_videoIndex = av_find_best_stream(m_pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (m_videoIndex >= 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ] video index:%d, fps = %0.2f\n", __func__, __LINE__,
               m_videoIndex, av_q2d(m_pFormatCtx->streams[m_videoIndex]->avg_frame_rate));
    }
    else
    {
        return -2;
    }

    return 0;
}

double AV_FFMPEG::fps()
{
    if (m_pFormatCtx)
        return av_q2d(m_pFormatCtx->streams[m_videoIndex]->avg_frame_rate);

    return -1;
}

double AV_FFMPEG::durationSec()
{
    if (m_pFormatCtx)
        return (double)m_pFormatCtx->duration / AV_TIME_BASE;

    return -1;
}

double AV_FFMPEG::playPosition()
{
    if (m_pFormatCtx && m_packet)
        return m_packet->pts * av_q2d(m_pFormatCtx->streams[m_packet->stream_index]->time_base);

    return -1;
}

void AV_FFMPEG::Close()
{
    avformat_close_input(&m_pFormatCtx);

    if (m_pFormatCtx)
        avformat_free_context(m_pFormatCtx);
    if (m_packet)
        av_packet_unref(m_packet);

    m_audioIndex = -1;
    m_videoIndex = -1;
    m_subtitleIndex = -1;

    return;
}

int AV_FFMPEG::PacketData(AV_PACKET_DATA *pkt)
{
    if (av_read_frame(m_pFormatCtx, m_packet) >= 0)
    {
        if (m_packet->stream_index == m_videoIndex)
        {
            if (m_packet->pts == AV_NOPTS_VALUE)
                m_packet->pts = 0;

            pkt->data_type = AV_TYPE_VIDEO;
            pkt->packet_data = m_packet->data;
            pkt->packet_size = m_packet->size;
            pkt->packet_pts = m_packet->pts;
            pkt->key_frame = m_packet->flags & AV_PKT_FLAG_KEY;

            return 0;
        }
        else if (m_packet->stream_index == m_audioIndex)
        {
            av_log(NULL, AV_LOG_INFO, "[ %s : %d ]audio \n", __func__, __LINE__);
            return 0;
        }
        else if (m_packet->stream_index == m_subtitleIndex)
        {
            av_log(NULL, AV_LOG_INFO, "[ %s : %d ]subtitle \n", __func__, __LINE__);
            return 0;
        }
    }

    return -1;
}

AVPacket *AV_FFMPEG::PacketData()
{
    if (av_read_frame(m_pFormatCtx, m_packet) >= 0)
    {
        return m_packet;
    }

    av_log(NULL, AV_LOG_INFO, "[ %s : %d ] GetPacketData error\n", __func__, __LINE__);

    return NULL;
}

AVPacket *AV_FFMPEG::PacketData(int &index)
{
    if (av_read_frame(m_pFormatCtx, m_packet) >= 0)
    {
        if (m_packet->stream_index == m_videoIndex)
        {
            index = m_videoIndex;
            return m_packet;
        }
        else if (m_packet->stream_index == m_audioIndex)
        {
            index = m_audioIndex;
            return m_packet;
        }
        else if (m_packet->stream_index == m_subtitleIndex)
        {
            index = m_subtitleIndex;
            return m_packet;
        }
        else
        {
            index = -1;
            av_log(NULL, AV_LOG_INFO, "[ %s : %d ] stream_index  = [ %d ] have not deal\n",
                   __func__, __LINE__, m_packet->stream_index);
        }
    }

    av_log(NULL, AV_LOG_INFO, "[ %s : %d ] PacketData ending -- \n", __func__, __LINE__);

    return NULL;
}

void AV_FFMPEG::freePacket()
{
    if (m_packet)
        av_packet_unref(m_packet);
}

AVFormatContext *AV_FFMPEG::AVFormatCtx()
{
    return m_pFormatCtx;
}

unsigned int AV_FFMPEG::NbStream()
{
    if (m_pFormatCtx)
        return m_pFormatCtx->nb_streams;
    return -1;
}

int AV_FFMPEG::VideoPixelFormat()
{
    if (m_pFormatCtx)
        return m_pFormatCtx->streams[m_videoIndex]->codecpar->format;
    return AV_PIX_FMT_NONE;
}

int AV_FFMPEG::VideoWidth()
{
    if (m_pFormatCtx)
        return m_pFormatCtx->streams[m_videoIndex]->codecpar->width;
    return -1;
}

int AV_FFMPEG::VideoHeight()
{
    if (m_pFormatCtx)
        return m_pFormatCtx->streams[m_videoIndex]->codecpar->height;
    return -1;
}

int AV_FFMPEG::VideoIndex()
{
    return m_videoIndex;
}

int AV_FFMPEG::AudioIndex()
{
    return m_audioIndex;
}

int AV_FFMPEG::SubtitleIndex()
{
    return m_subtitleIndex;
}