#include "AV_RTMP.h"
#include <unistd.h>
#include <stdlib.h>

AV_RTMP::AV_RTMP() : m_pRtmp(NULL), m_packet(NULL)
{
    RTMP_LogSetLevel(RTMP_LOGINFO);
}

AV_RTMP::~AV_RTMP()
{
    RTMPPacket_Free(m_packet);
    if (m_packet)
        delete m_packet;
}

void AV_RTMP::OnCreate()
{
    m_packet = new RTMPPacket();

    if (!RTMPPacket_Alloc(m_packet, 128 * 1024)) // 一般64K，这里64k不够
    {
        RTMP_Log(RTMP_LOGINFO, "[ %s : %d ] RTMPPacket_Alloc error\n", __func__, __LINE__);
        return;
    }

    RTMPPacket_Reset(m_packet);

    m_packet->m_hasAbsTimestamp = 0;
    m_packet->m_nChannel = 0x04; // control channel (invoke)
    m_packet->m_nInfoField2 = m_pRtmp->m_stream_id;
}

int AV_RTMP::Open(const char *pRtmpAddr)
{
    // 创建RTMP对象, 并进行初始化
    m_pRtmp = RTMP_Alloc();
    if (!m_pRtmp)
    {
        RTMP_Log(RTMP_LOGINFO, "[ %s : %d ] RTMP_Alloc error\n", __func__, __LINE__);
        return -1;
    }

    RTMP_Init(m_pRtmp);

    // 设置RTMP服务地址，以及设置连接超时时间
    m_pRtmp->Link.timeout = 10;
    printf("%s\n", (char *)pRtmpAddr);
    if (!RTMP_SetupURL(m_pRtmp, (char *)pRtmpAddr))
    {
        RTMP_Log(RTMP_LOGINFO, "[ %s : %d ] RTMP_SetupURL error\n", __func__, __LINE__);
        return -2;
    }

    // 设置了是推流(publish)，未设置是拉流(play)
    RTMP_EnableWrite(m_pRtmp);

    // 建立连接
    if (!RTMP_Connect(m_pRtmp, NULL))
    {
        RTMP_Log(RTMP_LOGINFO, "[ %s : %d ] RTMP_Connect error\n", __func__, __LINE__);
        return -3;
    }

    // 创建流
    if (!RTMP_ConnectStream(m_pRtmp, 0))
    {
        RTMP_Log(RTMP_LOGINFO, "[ %s : %d ] RTMP_ConnectStream error\n", __func__, __LINE__);
        return -4;
    }

    OnCreate();

    return 0;
}

void AV_RTMP::Close()
{
    RTMP_Close(m_pRtmp);
    if (m_pRtmp)
        RTMP_Free(m_pRtmp);
}

bool AV_RTMP::CheckConn()
{
    if (!RTMP_IsConnected(m_pRtmp))
    {
        RTMP_Log(RTMP_LOGINFO, "[ %s : %d ] RTMP is down ... \n", __func__, __LINE__);
        return false;
    }
    return true;
}

bool AV_RTMP::Publish()
{
    static unsigned int pre_ts = 0;
    if (CheckConn())
    {
        // if (m_packet->m_packetType != 0x08 && m_packet->m_packetType != 0x09)
        // {
        //     // 如果不是音频或视频类型
        //     printf("is not video or audio\n");
        //     return false;
        // }

        unsigned int diff = m_packet->m_nTimeStamp - pre_ts;
        usleep(diff * 1000 - 1000);

        if (!RTMP_SendPacket(m_pRtmp, m_packet, 0))
        {
            printf("RTMP_SendPacket error\n");
            return false;
        }
    }

    pre_ts = m_packet->m_nTimeStamp;

    return true;
}

RTMPPacket *&AV_RTMP::getRTMPPacket()
{
    return m_packet;
}