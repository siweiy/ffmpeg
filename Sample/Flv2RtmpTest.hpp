#pragma once

#include <signal.h>
#include "AV_RTMP/AV_RTMP.h"
#include "AV_Flv/AV_Flv.h"

#define HTON16(x) ((x >> 8 & 0xff) | (x << 8 & 0xff00))
#define HTON24(x) ((x >> 16 & 0xff) | (x << 16 & 0xff0000) | (x & 0xff00))
#define HTON32(x) ((x >> 24 & 0xff) | (x >> 8 & 0xff00) | \
                   (x << 8 & 0xff0000) | (x << 24 & 0xff000000))
#define HTONTIME(x) ((x >> 16 & 0xff) | (x << 16 & 0xff0000) | (x & 0xff00) | (x & 0xff000000))

#define TAB44 "    "
// #define PRINTF_DEBUG

static void DealTagHeader(unsigned char *const headerData, T_FLV_TAG_HEADER *tagHeader)
{
    static int videoTagNum = 0;
    static int audioTagNum = 0;

    unsigned char *data = NULL;

    T_FLV_TAG_HEADER header = {0};

    data = headerData;

    memset(&header, 0x0, sizeof(T_FLV_TAG_HEADER));

    header.type = data[0];

    data += 1;

    header.data_size = (data[0] << 16) | (data[1] << 8) | data[2];

    data += 3;

    header.timestamp = (data[0] << 16) | (data[1] << 8) | data[2];

    data += 3;

    header.timestamp_extended = data[0];

    data += 1;

    header.streamid = (data[0] << 16) | (data[1] << 8) | data[2];

    memcpy(tagHeader, &header, sizeof(T_FLV_TAG_HEADER));

#ifdef PRINTF_DEBUG
    switch (tagHeader->type)
    {
    case 0x12:
        printf("%s+Script Tag\n", TAB44);

        break;

    case 0x9:
        videoTagNum++;

        printf("%s+Video Tag[%d]\n", TAB44, videoTagNum);

        break;

    case 0x8:
        audioTagNum++;

        printf("%s+Audio Tag[%d]\n", TAB44, audioTagNum);

        break;

    default:
        break;
    }

    printf("%s%s+Tag Header\n", TAB44, TAB44);
    printf("%s%s%stype: %d, data_size: %d, timestamp: %d, timestamp_extended: %d, streamid: %d\n",
           TAB44, TAB44, TAB44, tagHeader->type, tagHeader->data_size, tagHeader->timestamp, tagHeader->timestamp_extended, tagHeader->streamid);
#endif
}

/** flv头数据
 * 一共9个字节
 * 1-3: signature: 'F' 'L' 'V'
 * 4: version : 1
 * 5: 0-5位，保留，必须是0
 * 5, 6位：是否有音频
 * 5, 7位：保留，必须是 0
 * 5, 8位：表示是否有视频 Tag
 * 6-9：header的大小，必须是9
*/
FILE *open_flv_file(const char *flv_name)
{
    FILE *fp = NULL;
    fp = fopen(flv_name, "rb");
    if (!fp)
    {
        printf("fopen %s error\n", flv_name);
        return NULL;
    }

    fseek(fp, 9, SEEK_SET); // 跳过flv头数据

    return fp;
}

bool flv_read_data(FILE *fp, RTMPPacket **pkt)
{

    /** PreTabSize 4字节
    */
    unsigned char preTagSizeData[5] = {0};
    memset(preTagSizeData, 0x0, sizeof(preTagSizeData));

    int dataLen = fread(preTagSizeData, 1, 4, fp);
    if (dataLen != 4)
    {
        exit(0);
    }

    preTagSizeData[4] = '\0';

#ifdef PRINTF_DEBUG
    printf("%spreviousTagSize: %d\n", TAB44, (preTagSizeData[0] << 24) | (preTagSizeData[1] << 16) | (preTagSizeData[2] << 8) | preTagSizeData[3]);
#endif

    /** tag header 11字节
     *  第一字节 TT(Tag Type), 0x08 音频, 0x09 视频, 0x12 script
     *  2-4, Tag body 的长度， PreTagSize - Tag Header size
     *  5-7, 时间戳，单位是毫秒; script 它的时间戳是0
     *  第8字节，扩展时间戳， 真正时间戳结构[扩展，时间戳] 一共4字节
     *  9-11, streamID, 0
    */
    T_FLV_TAG_HEADER tagHeader = {0};
    unsigned char tagHeaderData[12] = {0};
    memset(tagHeaderData, 0x0, sizeof(tagHeaderData));

    dataLen = fread(tagHeaderData, 1, 11, fp);
    if (dataLen != 11)
        exit(-1);

    memset(&tagHeader, 0x0, sizeof(T_FLV_TAG_HEADER));

    DealTagHeader(tagHeaderData, &tagHeader);

    memset((*pkt)->m_body, 0x0, tagHeader.data_size);
    if ((*pkt)->m_body == NULL)
    {
        printf("(*pkt)->m_body = NULL\n");
        exit(0);
    }

    if (fread((*pkt)->m_body, 1, tagHeader.data_size, fp) != tagHeader.data_size)
    {
        printf("fread (*pkt)->m_body has less\n");
        return false;
    }

    (*pkt)->m_headerType = RTMP_PACKET_SIZE_LARGE;
    (*pkt)->m_nBodySize = tagHeader.data_size;
    (*pkt)->m_packetType = tagHeader.type;
    (*pkt)->m_nTimeStamp = tagHeader.timestamp;

    return true;
}

//信号处理函数
void recvSignal(int sig)
{
    printf("received signal %d !!!\n", sig);
}

// 外部读取flv文件
void flv_rtmp_test1()
{
    FILE *flv_fp = NULL;
    RTMPPacket *pkt = NULL;
    AV_RTMP *av_rtmp = NULL;

    av_rtmp = new AV_RTMP();
    av_rtmp->Open("rtmp://localhost:1935/live/test");
    flv_fp = open_flv_file("Video/out.flv");
    if (!flv_fp)
        return;

    //给信号注册一个处理函数
    signal(SIGSEGV, recvSignal);

    while (true)
    {
        pkt = av_rtmp->getRTMPPacket();
        if (!pkt)
        {
            printf("pkt is null\n");
            break;
        }

        if (!flv_read_data(flv_fp, &pkt))
        {
            printf("flv_read_data error\n");
            break;
        }
        av_rtmp->Publish();
    }

    fclose(flv_fp);
    av_rtmp->Close();
}

// 使用AV_Flv类
void flv_rtmp_test2()
{
    RTMPPacket *pkt = NULL;
    AV_Flv *av_flv = NULL;
    AV_RTMP *av_rtmp = NULL;
    T_FLV_TAG_HEADER flvTagHeader;

    av_rtmp = new AV_RTMP();
    av_flv = new AV_Flv();

    av_rtmp->Open("rtmp://172.16.100.147:1935/live/test");
    av_flv->Open("Video/out.flv");

    //给信号注册一个处理函数
    // signal(SIGSEGV, recvSignal);

    while (true)
    {
        pkt = av_rtmp->getRTMPPacket();
        if (!pkt)
        {
            printf("pkt is null\n");
            break;
        }

        if (!av_flv->TagHeader(&flvTagHeader))
        {
            printf("av_flv->TagHeader error\n");
            break;
        }

        pkt->m_headerType = RTMP_PACKET_SIZE_LARGE;
        pkt->m_nBodySize = flvTagHeader.data_size;
        pkt->m_packetType = flvTagHeader.type;
        pkt->m_nTimeStamp = flvTagHeader.timestamp;

        if (!av_flv->TagBody((unsigned char **)&pkt->m_body))
        {
            printf("av_flv->TagBody error\n");
            break;
        }

        av_rtmp->Publish();
    }

    av_flv->Close();
    av_rtmp->Close();
}
