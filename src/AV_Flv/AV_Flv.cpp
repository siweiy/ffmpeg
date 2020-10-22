#include "AV_Flv.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;

#define TAB44 "    "
// #define PRINTF_DEBUG

void DealTagHeader(unsigned char *const headerData, T_FLV_TAG_HEADER *tagHeader);
void DealTagData(unsigned char *const tagData, const int tagType, const unsigned int tagSize);
void DealScriptTagData(unsigned char *const tagData, unsigned int tagDataSize);
void DealVideoTagData(unsigned char *const tagData);
void DealAudioTagData(unsigned char *const tagData);

AV_Flv::AV_Flv() : m_pFile(NULL)
{
    memset(&m_flvHeader, 0x0, sizeof(T_FLV_HEADER));
}

AV_Flv::~AV_Flv()
{
}

bool AV_Flv::Open(const char *flv_name)
{
    m_pFile = fopen(flv_name, "rb");
    if (!m_pFile)
    {
        printf("open file[%s] error!\n", flv_name);
        return false;
    }

    return flvHeader();
}

// 读Flv头: 9 byte
bool AV_Flv::flvHeader()
{
    unsigned char flvHeaderData[MIN_FLV_HEADER_LEN + 1] = {0};
    memset(flvHeaderData, 0x0, sizeof(flvHeaderData));

    int dataLen = fread(flvHeaderData, 1, MIN_FLV_HEADER_LEN, m_pFile);
    if (dataLen != MIN_FLV_HEADER_LEN)
    {
        printf("read flv header error!\n");
        fclose(m_pFile);

        return false;
    }

    flvHeaderData[MIN_FLV_HEADER_LEN] = '\0';

    unsigned char *data = NULL;

    data = flvHeaderData;

    memcpy(m_flvHeader.signature, data, MAX_SIGNATURE_LEN);

    m_flvHeader.signature[MAX_SIGNATURE_LEN] = '\0';

    data += MAX_SIGNATURE_LEN;

    m_flvHeader.version = data[0];

    data += 1;

    m_flvHeader.flags_audio = data[0] >> 2 & 0x1;
    m_flvHeader.flags_video = data[0] & 0x1;

    data += 1;

    m_flvHeader.headersize = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

    if (0x1 != m_flvHeader.version)
    {
        printf("version is not 1, todo...\n");
    }

#ifdef PRINTF_DEBUG
    printf("+FLV Header\n");
    printf("%ssignature: %s, version: %d, flags_audio: %d, flags_video: %d, headersize: %d\n",
           TAB44, m_flvHeader.signature, m_flvHeader.version, m_flvHeader.flags_audio,
           m_flvHeader.flags_video, m_flvHeader.headersize);
#endif

    return true;
}

T_FLV_HEADER AV_Flv::FlvHeader()
{
    return m_flvHeader;
}

bool AV_Flv::TagHeader(T_FLV_TAG_HEADER *tagHeader)
{
    unsigned char preTagSizeData[MAX_PRE_TAG_SIZE_LEN + 1] = {0};
    unsigned char tagHeaderData[MAX_TAG_HEADER_LEN + 1] = {0};

    memset(preTagSizeData, 0x0, sizeof(preTagSizeData));
    memset(tagHeaderData, 0x0, sizeof(tagHeaderData));

    int dataLen = fread(preTagSizeData, 1, MAX_PRE_TAG_SIZE_LEN, m_pFile);
    if (dataLen != MAX_PRE_TAG_SIZE_LEN)
    {
        return false;
    }

    preTagSizeData[MAX_PRE_TAG_SIZE_LEN] = '\0';

#ifdef PRINTF_DEBUG
    printf("%spreviousTagSize: %d\n", TAB44, (preTagSizeData[0] << 24) | (preTagSizeData[1] << 16) | (preTagSizeData[2] << 8) | preTagSizeData[3]);
#endif

    dataLen = fread(tagHeaderData, 1, MAX_TAG_HEADER_LEN, m_pFile);
    if (dataLen != MAX_TAG_HEADER_LEN)
    {
        return false;
    }

    memset(&m_tagHeader, 0x0, sizeof(T_FLV_TAG_HEADER));

    DealTagHeader(tagHeaderData, tagHeader);

    m_tagHeader = *tagHeader;

    return true;
}

bool AV_Flv::TagBody(unsigned char **pTagBody, bool toShow)
{
    if (m_tagHeader.data_size <= 0)
    {
        printf("TagBody: param size is zero \n");
        return false;
    }

    memset(*(pTagBody), 0x0, m_tagHeader.data_size);

    int dataLen = fread(*(pTagBody), 1, m_tagHeader.data_size, m_pFile);
    if (dataLen != m_tagHeader.data_size)
    {
        printf("read dataLen is error\n");
        return false;
    }
    
    if (toShow)
        DealTagData(*(pTagBody), m_tagHeader.type, m_tagHeader.data_size);

    return true;
}

void AV_Flv::Close()
{
    fclose(m_pFile);
}

void DealTagHeader(unsigned char *const headerData, T_FLV_TAG_HEADER *tagHeader)
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

void DealTagData(unsigned char *const tagData, const int tagType, const unsigned int tagSize)
{
#ifdef PRINTF_DEBUG
    printf("%s%s%s\n", TAB44, TAB44, "+Tag Data");
#endif

    switch (tagType)
    {
    case 0x12:
        DealScriptTagData(tagData, tagSize);

        break;

    case 0x9:
        DealVideoTagData(tagData);

        break;

    case 0x8:
        DealAudioTagData(tagData);

        break;

    default:
        break;
    }
}

/* 小端转double */
double dealAmfNumber(unsigned char *amfNum)
{
    double d = 0;

    unsigned char *dp = (unsigned char *)&d;

    dp[0] = amfNum[7];
    dp[1] = amfNum[6];
    dp[2] = amfNum[5];
    dp[3] = amfNum[4];
    dp[4] = amfNum[3];
    dp[5] = amfNum[2];
    dp[6] = amfNum[1];
    dp[7] = amfNum[0];

    return d;
}

/*
    第一个AMF包：
           第1个字节表示AMF包类型, 一般总是0x02, 表示字符串, 其他值表示意义请查阅文档.
           第2-3个字节为UI16类型值, 表示字符串的长度, 一般总是0x000A("onMetaData"长度).
           后面字节为字符串数据, 一般总为"onMetaData".

    第二个AMF包：
           第1个字节表示AMF包类型, 一般总是0x08, 表示数组.
           第2-5个字节为UI32类型值, 表示数组元素的个数.
           后面即为各数组元素的封装, 数组元素为元素名称和值组成的对. 表示方法如下:
           第1-2个字节表示元素名称的长度, 假设为L. 后面跟着为长度为L的字符串. 第L+3个字节表示元素值的类型.
           后面跟着为对应值, 占用字节数取决于值的类型.

    0 = Number type (double, 8)
    1 = Boolean type
    2 = String type
    3 = Object type
    4 = MovieClip type
    5 = Null type
    6 = Undefined type
    7 = Reference type
    8 = ECMA array type
    10 = Strict array type
    11 = Date type
    12 = Long string type

    1. 不要频繁的malloc小内存(内存碎片, 代价);
    2. 如该函数中arrayKey, arrayValue, amfStrData设置成指针, 然后malloc就有问题(字符串后残留上述三个最大长度中的字符);
    3. 可能的解释: 当用free释放的你用malloc分配的存储空间, 释放的存储空间并没有从进程的地址空间中删除, 而是保留在可用存储区池中,
       当再次用malloc时只要可用存储区池中有足够的地址空间, 都不会再向内可申请内存了, 而是在可用存储区池中分配了.

实际分析时: 8的数组后还有一串 00 00 09, 暂时不清楚, 先跳过if (tagDataSize <= 3)
*/
void DealScriptTagData(unsigned char *const tagData, unsigned int tagDataSize)
{
    int i = 0;
    int amfType = 0;
    int amfIndex = 0;
    int valueType = 0;
    int valueSize = 0;
    int keySize = 0;
    int arrayCount = 0;
    int amfStringSize = 0;

    double amfNum = 0;

    unsigned char amfStr[MAX_AMF_STR_SIZE + 1] = {0};

    unsigned char *data = NULL;

    data = tagData;

    for (;;)
    {
        if (tagDataSize <= 3)
        {
            break;
        }

        amfType = data[0];

        amfIndex += 1;

        data += 1;
        tagDataSize -= 1;

#ifdef PRINTF_DEBUG
        printf("%s%s%sAMF%d type: %d\n", TAB44, TAB44, TAB44, amfIndex, amfType);
#endif

        switch (amfType)
        {
        case 2:
            amfStringSize = (data[0] << 8) | data[1];

#ifdef PRINTF_DEBUG
            printf("%s%s%sAMF%d String size: %d\n", TAB44, TAB44, TAB44, amfIndex, amfStringSize);
#endif

            data += 2;
            tagDataSize -= 2;

            memset(amfStr, 0x0, sizeof(amfStr));

            memcpy(amfStr, data, amfStringSize);

            amfStr[amfStringSize] = '\0';

#ifdef PRINTF_DEBUG
            printf("%s%s%sAMF%d String: %s\n", TAB44, TAB44, TAB44, amfIndex, amfStr);
#endif

            data += amfStringSize;
            tagDataSize -= amfStringSize;

            break;

        case 8:
            arrayCount = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

#ifdef PRINTF_DEBUG
            printf("%s%s%sAMF%d Metadata count: %d\n", TAB44, TAB44, TAB44, amfIndex, arrayCount);
            printf("%s%s%s+Metadata\n", TAB44, TAB44, TAB44);
#endif

            data += 4;
            tagDataSize -= 4;

            for (i = 0; i < arrayCount; i++)
            {
                keySize = (data[0] << 8) | data[1];

                data += 2;
                tagDataSize -= 2;

                memset(amfStr, 0x0, sizeof(amfStr));

                memcpy(amfStr, data, keySize);

                amfStr[keySize] = '\0';

#ifdef PRINTF_DEBUG
                printf("%s%s%s%s%s: ", TAB44, TAB44, TAB44, TAB44, amfStr);
#endif

                data += keySize;
                tagDataSize -= keySize;

                valueType = data[0];

                data += 1;
                tagDataSize -= 1;

                if (0 == valueType)
                {
                    amfNum = dealAmfNumber(data);
#ifdef PRINTF_DEBUG
                    printf("%lf\n", amfNum);
#endif

                    data += 8;
                    tagDataSize -= 8;
                }
                else if (1 == valueType)
                {
#ifdef PRINTF_DEBUG
                    printf("%d\n", data[0]);
#endif
                    data += 1;
                    tagDataSize -= 1;
                }
                else if (2 == valueType)
                {
                    valueSize = (data[0] << 8) | data[1];

                    data += 2;
                    tagDataSize -= 2;

                    memset(amfStr, 0x0, sizeof(amfStr));

                    memcpy(amfStr, data, valueSize);

                    amfStr[valueSize] = '\0';

#ifdef PRINTF_DEBUG
                    printf("%s\n", amfStr);
#endif

                    data += valueSize;
                    tagDataSize -= valueSize;
                }
                else
                {
                    //printf("now can not parse value type: %d\n", valueType);

                    return;
                }
            }

            break;

        default:
            break;
        }
    }
}

/*
   Video Header = | FrameType(4) | CodecID(4) |
   VideoData = | FrameType(4) | CodecID(4) | VideoData(n) |
*/
void DealVideoTagData(unsigned char *const tagData)
{
    unsigned char *data = NULL;

    data = tagData;

    T_FLV_TAG_VIDEO_HEADER vTagHeader = {0};
    T_FLV_TAG_AVC_VIDEO_PACKET avcVideoPacket = {0};

    memset(&vTagHeader, 0x0, sizeof(vTagHeader));

    vTagHeader.freameType = data[0] >> 4 & 0xf;
    vTagHeader.codecId = data[0] & 0xf;

    data++;

#ifdef PRINTF_DEBUG
    printf("%s%s%sFrameType: %d\n", TAB44, TAB44, TAB44, vTagHeader.freameType);
    printf("%s%s%sCodecId: %d\n", TAB44, TAB44, TAB44, vTagHeader.codecId);
#endif

    /* now just avc(h264) */
    switch (vTagHeader.codecId)
    {
    case 0x07:
        memset(&avcVideoPacket, 0x0, sizeof(avcVideoPacket));

        avcVideoPacket.avcPacketType = data[0];
        avcVideoPacket.compositionTime = (data[1] << 16) | (data[2] << 8) | data[3];

        data += 4;

        if (0 == avcVideoPacket.avcPacketType)
        {
#ifdef PRINTF_DEBUG
            printf("%s%s%s+AVCVideoPacket\n", TAB44, TAB44, TAB44);
            printf("%s%s%s%sAVCPacketType: %d\n", TAB44, TAB44, TAB44, TAB44, avcVideoPacket.avcPacketType);
            printf("%s%s%s%sCompositionTime Offset: %d\n", TAB44, TAB44, TAB44, TAB44, avcVideoPacket.compositionTime);
#endif
            printf("%s%s%s%s+AVCDecoderConfigurationRecord\n", TAB44, TAB44, TAB44, TAB44);

            avcVideoPacket.vp.avcDecCfg.configurationVersion = data[0];
            avcVideoPacket.vp.avcDecCfg.AVCProfileIndication = data[1];
            avcVideoPacket.vp.avcDecCfg.profile_compatibility = data[2];
            avcVideoPacket.vp.avcDecCfg.AVCLevelIndication = data[3];
            avcVideoPacket.vp.avcDecCfg.lengthSizeMinusOne = data[4] & 0x3;
            avcVideoPacket.vp.avcDecCfg.numOfSequenceParameterSets = data[5] & 0x1f;
            avcVideoPacket.vp.avcDecCfg.spsLen = (data[6] << 8) | data[7];

            // todo, parse sps

            data += (8 + avcVideoPacket.vp.avcDecCfg.spsLen);

            avcVideoPacket.vp.avcDecCfg.numOfPictureParameterSets = data[0];
            avcVideoPacket.vp.avcDecCfg.ppsLen = (data[1] << 8) | data[2];

            // todo, parse pps

#ifdef PRINTF_DEBUG
            printf("%s%s%s%s%sconfigurationVersion: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.configurationVersion);
            printf("%s%s%s%s%sAVCProfileIndication: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.AVCProfileIndication);
            printf("%s%s%s%s%sprofile_compatibility: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.profile_compatibility);
            printf("%s%s%s%s%sAVCLevelIndication: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.AVCLevelIndication);
            printf("%s%s%s%s%slengthSizeMinusOne: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.lengthSizeMinusOne);
            printf("%s%s%s%s%snumOfSequenceParameterSets: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.numOfSequenceParameterSets);
            printf("%s%s%s%s%ssequenceParameterSetLength: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.spsLen);
            printf("%s%s%s%s%snumOfPictureParameterSets: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.numOfPictureParameterSets);
            printf("%s%s%s%s%spictureParameterSetLength: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, avcVideoPacket.vp.avcDecCfg.ppsLen);
#endif
        }
        else
        {
#ifdef PRINTF_DEBUG
            printf("%s%s%s+Video Data\n", TAB44, TAB44, TAB44);
            printf("%s%s%s%sAVCPacketType: %d\n", TAB44, TAB44, TAB44, TAB44, avcVideoPacket.avcPacketType);
            printf("%s%s%s%sCompositionTime Offset: %d\n", TAB44, TAB44, TAB44, TAB44, avcVideoPacket.compositionTime);
            printf("%s%s%s%sData\n", TAB44, TAB44, TAB44, TAB44);
#endif
        }

        break;

    default:
        break;
    }
}

void DealAudioTagData(unsigned char *const tagData)
{
    unsigned char *data = NULL;

    data = tagData;

    T_FLV_TAG_AUDIO_HEADER audioHeader = {0};
    T_FLV_TAG_AAC_AUDIO_PACKET aPacket = {0};

    memset(&audioHeader, 0x0, sizeof(T_FLV_TAG_AUDIO_HEADER));

    audioHeader.soundFormat = (data[0] >> 4) & 0xf;
    audioHeader.soundRate = (data[0] >> 2) & 0x3;
    audioHeader.soundSize = (data[0] >> 1) & 0x1;
    audioHeader.soundType = data[0] & 0x1;

#ifdef PRINTF_DEBUG
    printf("%s%s%sSoundFormat: %d\n", TAB44, TAB44, TAB44, audioHeader.soundFormat);

    switch (audioHeader.soundRate)
    {
    case 0:
        printf("%s%s%sSoundRate: 5.5-KHz\n", TAB44, TAB44, TAB44);
        break;

    case 1:
        printf("%s%s%sSoundRate: 11-KHz\n", TAB44, TAB44, TAB44);
        break;

    case 2:
        printf("%s%s%sSoundRate: 22-KHz\n", TAB44, TAB44, TAB44);
        break;

    case 3:
        printf("%s%s%sSoundRate: 44-KHz\n", TAB44, TAB44, TAB44);
        break;

    default:
        printf("%s%s%sSoundRate: %d\n", TAB44, TAB44, TAB44, audioHeader.soundRate);
    }

    switch (audioHeader.soundSize)
    {
    case 0:
        printf("%s%s%sSoundSize: snd8bit\n", TAB44, TAB44, TAB44);
        break;

    case 1:
        printf("%s%s%sSoundSize: snd16bit\n", TAB44, TAB44, TAB44);
        break;

    default:
        printf("%s%s%sSoundSize: %d\n", TAB44, TAB44, TAB44, audioHeader.soundSize);
    }

    switch (audioHeader.soundType)
    {
    case 0:
        printf("%s%s%sSoundType: sndMono\n", TAB44, TAB44, TAB44);
        break;

    case 1:
        printf("%s%s%sSoundType: sndStereo\n", TAB44, TAB44, TAB44);
        break;

    default:
        printf("%s%s%sSoundSize: %d\n", TAB44, TAB44, TAB44, audioHeader.soundSize);
    }
#endif

    data++;

    /* now just for aac */
    switch (audioHeader.soundFormat)
    {
    case 0xa:
        memset(&aPacket, 0x0, sizeof(T_FLV_TAG_AAC_AUDIO_PACKET));

        aPacket.aacPacketType = data[0];

        if (0 == aPacket.aacPacketType)
        {
#ifdef PRINTF_DEBUG
            printf("%s%s%s+AACAudioData\n", TAB44, TAB44, TAB44);
            printf("%s%s%s%sAACPacketType: %d\n", TAB44, TAB44, TAB44, TAB44, aPacket.aacPacketType);
#endif
            aPacket.ap.aacSpecCfg.audioObjectType = (data[1] >> 3) & 0x1f;
            aPacket.ap.aacSpecCfg.samplingFreqIndex = ((data[1] & 0x7) << 1) | ((data[2] >> 7) & 0x1);
            aPacket.ap.aacSpecCfg.channelCfg = (data[2] >> 3) & 0xf;

#ifdef PRINTF_DEBUG
            printf("%s%s%s%s+AudioSpecificConfig\n", TAB44, TAB44, TAB44, TAB44);

            printf("%s%s%s%s%sAudioObjectType: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, aPacket.ap.aacSpecCfg.audioObjectType);
            printf("%s%s%s%s%sSamplingFrequencyIndex: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, aPacket.ap.aacSpecCfg.samplingFreqIndex);
            printf("%s%s%s%s%sChannelConfiguration: %d\n", TAB44, TAB44, TAB44, TAB44, TAB44, aPacket.ap.aacSpecCfg.channelCfg);
#endif
        }
        else
        {
#ifdef PRINTF_DEBUG
            printf("%s%s%s+AACAudioData\n", TAB44, TAB44, TAB44);
            printf("%s%s%s%sAACPacketType: %d\n", TAB44, TAB44, TAB44, TAB44, aPacket.aacPacketType);
            printf("%s%s%s%sData(Raw AAC frame data)\n", TAB44, TAB44, TAB44, TAB44);
#endif
        }

        break;

    default:
        break;
    }
}