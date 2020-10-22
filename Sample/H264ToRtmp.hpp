#pragma once

#include <assert.h>
#include <signal.h>
#include "Singleton.hpp"
#include "AV_RTMP/AV_RTMP.h"
#include "AV_FFMPEG/AV_FFMPEG.h"
#include "AV_Vdec/AV_Vdec.h"
#include "AV_FrameConvert/AV_FrameConvert.h"

#define RTMP_ADDR "rtmp://127.0.0.1:1935/live/test"
#define RTSP_ADDR "rtsp://admin:ematech123@172.16.98.223:554/h264/ch1/main/av_stream"

class H264ToRTMP
{
public:
    H264ToRTMP();
    ~H264ToRTMP();

    void Install(const char *rtsp_addr, const char *rtmp_addr);
    void Push();
    void Release();

private:


private:
    AV_RTMP *m_Avrtmp;
    AV_FFMPEG *m_Avffmpeg;
    AV_Vdec *m_Avvdec;
    AV_FrameConvert *m_Avconvert;
    RTMPPacket *m_Rtmppkt;
    AVPacket *m_Avpkt;
};

H264ToRTMP::H264ToRTMP()
{
    m_Avrtmp = new AV_RTMP();
    m_Avffmpeg = new AV_FFMPEG();
    m_Avvdec = new AV_Vdec();
    m_Avconvert = new AV_FrameConvert();
}

H264ToRTMP::~H264ToRTMP()
{
    delete m_Avrtmp;
    delete m_Avffmpeg;
    delete m_Avvdec;
    delete m_Avconvert;
}

void H264ToRTMP::Install(const char *rtsp_addr, const char *rtmp_addr)
{
    assert(m_Avrtmp->Open(rtmp_addr) == 0);
    assert(m_Avffmpeg->Open(rtsp_addr) == 0);
    assert(m_Avvdec->Open(m_Avffmpeg->AVFormatCtx(), m_Avffmpeg->VideoIndex()) == 0);
    assert(m_Avconvert->Open(AV_PIX_FMT_YUV420P, SWS_BICUBIC, m_Avvdec->AVCodecCtx()) == true);
}

void H264ToRTMP::Push()
{
    // 得到H264数据
    m_Avpkt = m_Avffmpeg->PacketData();

    // 配置rtmp格式数据
    m_Rtmppkt = m_Avrtmp->getRTMPPacket();
    // m_Rtmppkt->m_headerType = RTMP_PACKET_SIZE_LARGE;
    // m_Rtmppkt->m_nBodySize = flvTagHeader.data_size;
    // m_Rtmppkt->m_packetType = flvTagHeader.type;
    // m_Rtmppkt->m_nTimeStamp = flvTagHeader.timestamp;
    // m_Rtmppkt->m_body

    m_Avrtmp->Publish();
}

void H264ToRTMP::Release()
{
    m_Avrtmp->Close();
    m_Avffmpeg->Close();
    m_Avvdec->Close();
    m_Avconvert->Close();
}

void handle(int signo)
{
    Singleton<H264ToRTMP>::Instance()->Release();
    Singleton<H264ToRTMP>::Release();

    exit(0);
}

int h264ToRtmp()
{
    signal(SIGINT, handle);

    // 测试单例模板
    Singleton<H264ToRTMP>::Instance()->Install(RTSP_ADDR, RTMP_ADDR);

    while (1)
    {
        Singleton<H264ToRTMP>::Instance()->Push();
        cout << "推流中 <<成功>> ...." << endl;
    }

    Singleton<H264ToRTMP>::Instance()->Release();
    Singleton<H264ToRTMP>::Release();

    return 0;
}
