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

    /**
     * @brief Open (video file、rtsp)
     * @param rtsp_url： addr or file
     * @param rtsp: is rtsp?，if not, please input false
     * @return: 0 success
    */
    int Open(const char *rtsp_url, bool rtsp = true);

    /**
     * @brief Open
     * @param dev : device name
     * @param video_fmt: device format,ep:v4l2
     * @return: 0 success
    */
    int Open(const char *dev, const char *video_fmt);

    /*  Close video stream

        @param dev : device name
        @param video_fmt: device format,ep:v4l2
    */
    void Close();

    /**
     * @brief  Get video frame data in packet
     * @param
     *   AV_PACKET_DATA-->
     *   packet_data: video frame data
     *   packet_size: video frame size
     *   packet_pts : AVStream->time_base
     *   key_frame  : Keyframes
     *   data_type  : video 、audio or text
    */
    int PacketData(AV_PACKET_DATA *pkt);

    /*
        Don't do processing, return all streams
    */
    AVPacket *PacketData();

    /*
        @param index: current stream index 
    */
    AVPacket *PacketData(int &index);

    /**
     * @brief Free packet memory
    */
    void freePacket();

    AVFormatContext *AVFormatCtx();

    unsigned int NbStream();
    int VideoPixelFormat();

    int VideoWidth();
    int VideoHeight();

    int VideoIndex();
    int AudioIndex();
    int SubtitleIndex();

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