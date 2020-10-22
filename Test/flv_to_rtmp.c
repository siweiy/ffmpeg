#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "librtmp/rtmp.h"
#include "librtmp/amf.h"
#include "librtmp/http.h"
#include "librtmp/log.h"

#define uint32_t unsigned int

#define HTON16(x) ((x >> 8 & 0xff) | (x << 8 & 0xff00))
#define HTON24(x) ((x >> 16 & 0xff) | (x << 16 & 0xff0000) | (x & 0xff00))
#define HTON32(x) ((x >> 24 & 0xff) | (x >> 8 & 0xff00) | \
                   (x << 8 & 0xff0000) | (x << 24 & 0xff000000))
#define HTONTIME(x) ((x >> 16 & 0xff) | (x << 16 & 0xff0000) | (x & 0xff00) | (x & 0xff000000))

/*read 1 byte*/
int ReadU8(uint32_t *u8, FILE *fp)
{
    if (fread(u8, 1, 1, fp) != 1)
        return 0;
    return 1;
}
/*read 2 byte*/
int ReadU16(uint32_t *u16, FILE *fp)
{
    if (fread(u16, 2, 1, fp) != 1)
        return 0;
    *u16 = HTON16(*u16);
    return 1;
}
/*read 3 byte*/
int ReadU24(uint32_t *u24, FILE *fp)
{
    if (fread(u24, 3, 1, fp) != 1)
        return 0;
    *u24 = HTON24(*u24);
    return 1;
}
/*read 4 byte*/
int ReadU32(uint32_t *u32, FILE *fp)
{
    if (fread(u32, 4, 1, fp) != 1)
        return 0;
    *u32 = HTON32(*u32);
    return 1;
}
/*read 1 byte,and loopback 1 byte at once*/
int PeekU8(uint32_t *u8, FILE *fp)
{
    if (fread(u8, 1, 1, fp) != 1)
        return 0;
    fseek(fp, -1, SEEK_CUR);
    return 1;
}
/*read 4 byte and convert to time format*/
int ReadTime(uint32_t *utime, FILE *fp)
{
    if (fread(utime, 4, 1, fp) != 1)
        return 0;
    *utime = HTONTIME(*utime);
    return 1;
}

int push_packet(char *infile, char *rtmp_server_path)
{
    RTMP *rtmp = NULL;
    RTMPPacket *packet = NULL;
    uint32_t start_time = 0;
    uint32_t now_time = 0;
    //上一帧时间戳
    long pre_frame_time = 0;
    long lasttime = 0;
    //下一帧是否是关键帧
    int bNextIsKey = 1;
    uint32_t preTagSize = 0;
    uint32_t type = 0;
    uint32_t datalength = 0;
    uint32_t timestamp = 0;
    uint32_t streamid = 0;
    FILE *fp = NULL;
    fp = fopen(infile, "rb");
    if (!fp)
    {
        RTMP_LogPrintf("Open File Error\n");
        return -1;
    }

    //为结构体“RTMP”分配内存
    rtmp = RTMP_Alloc();
    if (!rtmp)
    {
        RTMP_LogPrintf("RTMP_Alloc failed\n");
        return -1;
    }
    //初始化结构体“RTMP”中的成员变量
    RTMP_Init(rtmp);
    rtmp->Link.timeout = 5;
    if (!RTMP_SetupURL(rtmp, rtmp_server_path))
    {
        RTMP_Log(RTMP_LOGERROR, "SetupURL Err\n");
        RTMP_Free(rtmp);
        return -1;
    }
    //发布流的时候必须要使用。如果不使用则代表接收流。
    RTMP_EnableWrite(rtmp);
    //建立RTMP连接，创建一个RTMP协议规范中的NetConnection
    if (!RTMP_Connect(rtmp, NULL))
    {
        RTMP_Log(RTMP_LOGERROR, "Connect Err\n");
        RTMP_Free(rtmp);
        return -1;
    }
    //创建一个RTMP协议规范中的NetStream
    if (!RTMP_ConnectStream(rtmp, 0))
    {
        RTMP_Log(RTMP_LOGERROR, "ConnectStream Err\n");
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        return -1;
    }

    packet = (RTMPPacket *)malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, 1024 * 64);
    RTMPPacket_Reset(packet);
    packet->m_hasAbsTimestamp = 0;
    packet->m_nChannel = 0x04;
    packet->m_nInfoField2 = rtmp->m_stream_id;
    RTMP_LogPrintf("Start to send data ...\n");
    //跳过flv的9个头字节
    fseek(fp, 9, SEEK_SET);
    //跳过previousTagSize所占的4个字节
    fseek(fp, 4, SEEK_CUR);
    start_time = RTMP_GetTime();
    while (1)
    {
        //如果下一帧是关键帧，且上一帧的时间戳比现在的时间还长(外部时钟)，说明推流速度过快了，可以延时下
        if ((((now_time = RTMP_GetTime()) - start_time) < (pre_frame_time)) && bNextIsKey)
        {
            //wait for 1 sec if the send process is too fast
            //这个机制不好，需要改进
            if (pre_frame_time > lasttime)
            {
                RTMP_LogPrintf("TimeStamp:%8lu ms\n", pre_frame_time);
                lasttime = pre_frame_time;
            }
            sleep(1); //等待1秒
            continue;
        }
        //not quite the same as FLV spec
        if (!ReadU8(&type, fp)) //读取type
        {
            printf("11111111111111111111111\n");
            break;
        }
        if (!ReadU24(&datalength, fp)) //读取datalength的长度
        {
            printf("2222222222222222222222222\n");
            break;
        }
        if (!ReadTime(&timestamp, fp)) //从flv的head读取时间戳
        {
            printf("33333333333333333333333\n");
            break;
        }
        if (!ReadU24(&streamid, fp)) //读取streamid的类型
        {
            printf("444444444444444444444\n");
            break;
        }
        if (type != 0x08 && type != 0x09)
        { //如果不是音频或视频类型
            //jump over non_audio and non_video frame，
            //jump over next previousTagSizen at the same time
            fseek(fp, datalength + 4, SEEK_CUR); //跳过脚本以及脚本后的previousTagSize
            continue;
        }
        printf("datalength = %d\n", datalength);
        //把flv的音频和视频数据写入到packet的body中
        int ret;
        if ((ret = fread(packet->m_body, 1, datalength, fp)) != datalength)
        {
            printf("ret = %d\n", ret);
            printf("555555555555555555\n");
            break;
        }
        //packet header  大尺寸
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        packet->m_nTimeStamp = timestamp; //时间戳
        packet->m_packetType = type;      //包类型
        packet->m_nBodySize = datalength; //包大小
        pre_frame_time = timestamp;       //把包读取到的时间戳赋值给上一帧时间戳变量
        if (!RTMP_IsConnected(rtmp))
        { //确认连接
            RTMP_Log(RTMP_LOGERROR, "rtmp is not connect\n");
            break;
        }
        RTMP_Log(RTMP_LOGINFO, "send packet\n");
        //发送一个RTMP数据RTMPPacket
        if (!RTMP_SendPacket(rtmp, packet, 0))
        {
            printf("666666666666666666\n");
            break;
        }
        if (!ReadU32(&preTagSize, fp))
        {
            printf("777777777777777777\n");
            break;
        }
        if (!PeekU8(&type, fp)) //去读下一帧的类型
        {
            printf("888888888888888888888\n");
            break;
        }

        if (type == 0x09)
        { //如果下一帧是视频
            if (fseek(fp, 11, SEEK_CUR) != 0)
                break;
            if (!PeekU8(&type, fp))
            {
                break;
            }
            if (type == 0x17) //如果视频帧是关键帧
                bNextIsKey = 1;
            else
                bNextIsKey = 0;
            fseek(fp, -11, SEEK_CUR);
        }

        usleep(50000);
    }
    RTMP_LogPrintf("\nSend Data Over!\n");

    if (fp)
    {
        fclose(fp);
    }
    if (rtmp != NULL)
    {
        //关闭RTMP连接
        RTMP_Close(rtmp);
        //释放结构体RTMP
        RTMP_Free(rtmp);
        rtmp = NULL;
    }
    if (packet != NULL)
    {
        RTMPPacket_Free(packet);
        free(packet);
        packet = NULL;
    }
    // 关闭Socket
    return 0;
}

int main(int argc, char **argv)
{
    //设置log为info级别
    RTMP_debuglevel = RTMP_LOGINFO;
    char *infile = "out.flv";
    char *rtmpserverpath = "rtmp://localhost:1935/live/test";
    push_packet(infile, rtmpserverpath);
    
    return 0;
}
