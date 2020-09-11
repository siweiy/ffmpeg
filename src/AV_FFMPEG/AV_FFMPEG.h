#pragma once

/*  版本包 ffmpeg-4.3.1.tar.xz
    获取rtsp 或者 dev设备的视频流数据
    获取packet
*/

#include "AV_Header.h"

extern "C"
{
#include "libavdevice/avdevice.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"
}

class AV_FFMPEG
{
public:
    AV_FFMPEG();
    ~AV_FFMPEG();

    /*
        open (video file、rtsp or dev)
    */
    int Open(const char *rtsp_url, bool rtsp=true);
    int Open(const char *dev, const char *video_fmt);
    void Close();

    /*  Get video frame data in packet

        @param
        AV_PACKET_DATA-->
        packet_data: video frame data
        packet_size: video frame size
        packet_pts : AVStream->time_base
        key_frame  : Keyframes
        data_type  : video 、audio or text
    */
    int GetPacketData(AV_PACKET_DATA *pkt);
    AVPacket* GetPacketData();
    AVPacket* GetPacketData(int &index);

    /*
        Free packet memory
    */
    void freePacket();

    AVFormatContext* GetAVFormatContext();
    int GetVideoIndex();
    int GetAudioIndex();
    int GetSubtitleIndex();
    double fps();
    double durationSec();
    double playPosition();

private:
    void AVInit();
    int AVMemberInit();
    int findStreamIndex();

private:
    AVFormatContext *m_pFormatCtx;
    AVPacket *m_packet;
    int m_videoIndex;
    int m_audioIndex;
    int m_subtitleIndex;
};