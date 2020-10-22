#pragma once

extern "C"
{
#include <string.h>
#include "librtmp/rtmp.h"
#include "librtmp/amf.h"
#include "librtmp/http.h"
#include "librtmp/log.h"
}

class AV_RTMP
{
public:
    AV_RTMP();
    ~AV_RTMP();

    /**
     * @brief Open Rtmp
     * @param pRtmpAddr:rtmp server address.
     *  
     */
    int Open(const char *pRtmpAddr);

    /**
     * @brief Close
     * 
     */
    void Close();

    /**
     * @brief checked inline status
     * @return connected status
     */
    bool CheckConn();

    /**
     * @brief Push rtmp stream
     * @param 
     * 
     */
    bool Publish();

    RTMPPacket *&getRTMPPacket();

private:
    void OnCreate();

private:
    RTMP *m_pRtmp;
    RTMPPacket *m_packet;
};