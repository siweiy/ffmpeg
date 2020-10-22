/**
 * 编译生成解析flv程序打印输出
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define TAB44 "    "
#define PRINTF_DEBUG

#define MAX_SIGNATURE_LEN 3
#define MAX_PRE_TAG_SIZE_LEN 4
#define MIN_FLV_HEADER_LEN 9
#define MAX_TAG_HEADER_LEN 11
#define MAX_PARSE_TAG_NUM 15
#define MAX_AMF_STR_SIZE 255

/************************************************************************************************************
**                                        flv header: 记录了flv的类型, 版本等信息, 是flv的开头, 一般都差不多, 占9bytes
**
-------------------------------------------------------------------------------------------------------------
**        字段名称            　　 |    长度(bytes)    |        有关描述
-------------------------------------------------------------------------------------------------------------
**        signature              |    3             |        文件标识, 总是为"FLV", 0x46 0x4c 0x56
**        version                |    1             |        版本(目前为0x01)
**        flag                   |    3             |        文件的标志位说明. 前5位保留, 必须为0;
                                                             第6位为音频Tag: 1表示有音频; 第七位保留, 为0; 第8位为视频Tag: 1表示有视频
**        headersize             |    4             |        整个header的长度, 一般为9(版本为0x01时); 大于9表示下面还有扩展信息
************************************************************************************************************/
/*
   1. unsigned char reserved5: 5, flags_audio: 1, reserved1: 1, flags_video: 1;
   2. unsigned char : 5, flags_audio: 1, : 1, flags_video: 1; (无名说明无法使用, 仅占位)
   3. 下面结构体位域的另外两种写法.
*/
typedef struct t_flv_header
{
    unsigned char signature[MAX_SIGNATURE_LEN + 1];
    unsigned char version;
    unsigned char : 5;
    unsigned char flags_audio : 1;
    unsigned char : 1;
    unsigned char flags_video : 1;

    int headersize;
} T_FLV_HEADER;

/************************************************************************************************************
**                                        tag header
**
-------------------------------------------------------------------------------------------------------------
**        字段名称            　　 |    长度(bytes)    |        有关描述
-------------------------------------------------------------------------------------------------------------
**        type                   |    1             |        数据类型, (0x12)为脚本类型; (0x08)为音频类型; (0x09）为视频类型
**        data_size              |    3             |        数据区长度
**        timestamp              |    3             |        时间戳, 类型为(0x12)的tag时间戳一直为0, (0xFFFFFF)可以表示长度为4小时, 单位为毫秒.
**        timestamp_extended     |    1             |        将时间戳扩展为4bytes, 代表高8位, 一般都为0, 长度为4小时的flv一般很少见了
**        streamid               |    3             |        总为0
************************************************************************************************************/
typedef struct t_flv_tag_header
{
    int type;
    int data_size;
    int timestamp;
    int timestamp_extended;
    int streamid;
} T_FLV_TAG_HEADER;

/************************************************************************************************************
**                                        video tag header
**
-------------------------------------------------------------------------------------------------------------
**        字段名称            　　 |    长度(bytes)    |        有关描述
-------------------------------------------------------------------------------------------------------------
**        FreameType             |    4(bits)             |  FrameType为数据类型, 1为关键帧, 2为非关键帧, 3为h263的非关键帧,
                                                             4为服务器生成关键帧, 5为视频信息或命令帧.
**        CodecId                |    4(bits)             |  CodecID为包装类型, 1为JPEG, 2为H263, 3为Screen video,
                                                             4为On2 VP6, 5为On2 VP6, 6为Screen videoversion 2, 7为AVC

CodecID=2, 为H263VideoPacket;
CodecID=3, 为ScreenVideopacket;
CodecID=4, 为VP6FLVVideoPacket;
CodecID=5, 为VP6FLVAlphaVideoPacket;
CodecID=6, 为ScreenV2VideoPacket;
CodecID=7, 为AVCVideoPacket.

** AVCVideoPacket Format: | AVCPacketType(8)| CompostionTime(24) | Data |
    如果AVCPacketType=0x00, 为AVCSequence Header;--描述了SPS以及PPS的信息;
    如果AVCPacketType=0x01, 为AVC NALU;
    如果AVCPacketType=0x02, 为AVC end ofsequence;
    CompositionTime为相对时间戳: 如果AVCPacketType=0x01, 为相对时间戳; 其它, 均为0;

   Data为负载数据:
    如果AVCPacketType=0x00, 为AVCDecorderConfigurationRecord;
    如果AVCPacketType=0x01, 为NALUs;
    如果AVCPacketType=0x02, 为空.

   AVCDecorderConfigurationRecord格式, 包括文件的信息:
    | cfgVersion(8) | avcProfile(8) | profileCompatibility(8) |avcLevel(8) | reserved(6) | lengthSizeMinusOne(2) | reserved(3) | numOfSPS(5) |spsLength(16) | sps(n) | numOfPPS(8) | ppsLength(16) | pps(n) |
************************************************************************************************************/
typedef struct t_flv_tag_video_header
{
    unsigned char freameType : 4, codecId : 4;
} T_FLV_TAG_VIDEO_HEADER;

/************************************************************************************************************
**                                        AVCDecoderConfigurationRecord
**
-------------------------------------------------------------------------------------------------------------
**        字段名称            　　 |    长度(bytes)    |        有关描述
-------------------------------------------------------------------------------------------------------------
**        configurationVersion   |    1             |        配置版本占用8位, 一定为1
**        AVCProfileIndication   |    1             |        profile_idc占用8位, 从H.264标准SPS第一个字段profile_idc拷贝而来, 指明所用profile
**        profile_compatibility  |    1             |        占用8位, 从H.264标准SPS拷贝的冗余字
**        AVCLevelIndication     |    1             |        level_idc占用8位, 从H.264标准SPS第一个字段level_idc拷贝而来, 指明所用 level
**        reserved               |    6b            |        保留位占6位, 值一定为'111111'
**        lengthSizeMinusOne     |    2b            |        占用2位, 表示NAL单元头的长度, 0表示1字节, 1表示2字节, 2表示3字节, 3表示4字节
**        reserved               |    3b            |        保留位占3位, 值一定为'111'
**        numOfSPS               |    5b            |        numOfSequenceParameterSets占用5位, 表示当前SPS的个数
**        SPSLength               |    2             |        sequenceParameterSetLength占用16位, SPS占用的长度
**        SPSData                   |    *             |
**        numOfPPS               |    5b            |        numOfPictureParameterSets占用8位, 表示当前PPS的个数
**        PPSLength               |    2             |        pictureParameterSetLength占用16位, PPS占用的长度
**        PPSData                    |    *             |        numOfPictureParameterSets占用8位, 表示当前PPS的个数

AVCProfileIndication, profile_compatibility, AVCLevelIndication就是拷贝SPS的前3个字节
************************************************************************************************************/
typedef struct t_flv_tag_avc_dec_cfg
{
    unsigned char configurationVersion;
    unsigned char AVCProfileIndication;
    unsigned char profile_compatibility;
    unsigned char AVCLevelIndication;
    unsigned char : 6, lengthSizeMinusOne : 2;

    unsigned char : 3, numOfSequenceParameterSets : 5;
    unsigned short spsLen;
    unsigned char *spsData;

    unsigned char numOfPictureParameterSets;
    unsigned short ppsLen;
    unsigned char *ppsData;
} T_FLV_TAG_AVC_DEC_CFG;

/************************************************************************************************************
**                                        avc video packet header
**
-------------------------------------------------------------------------------------------------------------
**        字段名称            　　 |    长度(bytes)    |        有关描述
-------------------------------------------------------------------------------------------------------------
**        AVCPacketType占用1字节 |    1                |
**        CompositionTime        |    3             |

AVCVideoPacket同样包括Packet Header和Packet Body两部分：
Packet Header:
        AVCPacketType占用1字节, 仅在AVC时有此字段
               0, AVC sequence header (SPS、PPS信息等)
               1, AVC NALU
               2, AVC end of sequence (lower level NALU sequence ender is not required or supported)

        CompositionTime占用24位, 相对时间戳, 如果AVCPacketType=0x01为相对时间戳; 其它, 均为0;
        该值表示当前帧的显示时间, tag的时间为解码时间, 显示时间等于 解码时间+CompositionTime.
************************************************************************************************************/
typedef struct t_flv_tag_avc_video_packet
{
    unsigned char avcPacketType;

    int compositionTime;

    union videoPacket
    {
        T_FLV_TAG_AVC_DEC_CFG avcDecCfg;
    } vp;
} T_FLV_TAG_AVC_VIDEO_PACKET;

typedef struct t_flv_tag_audio_header
{
    unsigned char soundFormat : 4, soundRate : 2, soundSize : 1, soundType : 1;
} T_FLV_TAG_AUDIO_HEADER;

typedef struct t_flv_tag_aac_spec_cfg
{
    unsigned char audioObjectType : 5;
    unsigned char samplingFreqIndex : 4, channelCfg : 2;
} T_FLV_TAG_AAC_SPEC_CFG;

typedef struct t_flv_tag_aac_audio_packet
{
    unsigned char aacPacketType;

    union audioPacket
    {
        T_FLV_TAG_AAC_SPEC_CFG aacSpecCfg;
    } ap;
} T_FLV_TAG_AAC_AUDIO_PACKET;

typedef struct t_flv_tag
{
} T_FLV_TAG;

/* 小端转double */
static double dealAmfNumber(unsigned char *amfNum)
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
  1. DealHeader(const unsigned char* headerData);
     这样定义会报warning: assignment discards 'const' qualifier from pointer target type,
     大意是指针丢掉"const"限定符.
  2. 原因是: data = headerData; 这一句存在丢掉的风险(可通过给*data赋予不同的值, 使得headerData的数据也被修改, 失去const的作用)
  3. const int *p; //这种情况表示*p是const无法进行修改, 而p是可以进行修改的;
     int* const p; //这种情况表示p是const无法进行修改, 而*p是可以进行修改的;
     const int* const p; //这种情况表示*p与p都无法进行修改.
*/
static void DealFlvHeader(unsigned char *const headerData)
{
    unsigned char *data = NULL;

    T_FLV_HEADER flvHeader = {0};

    data = headerData;

    memset(&flvHeader, 0x0, sizeof(T_FLV_HEADER));

    memcpy(flvHeader.signature, data, MAX_SIGNATURE_LEN);

    flvHeader.signature[MAX_SIGNATURE_LEN] = '\0';

    data += MAX_SIGNATURE_LEN;

    flvHeader.version = data[0];

    data += 1;

    flvHeader.flags_audio = data[0] >> 2 & 0x1;
    flvHeader.flags_video = data[0] & 0x1;

    data += 1;

    flvHeader.headersize = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

    if (0x1 != flvHeader.version)
    {
        printf("version is not 1, todo...\n");
    }

#ifdef PRINTF_DEBUG
    printf("+FLV Header\n");
    printf("%ssignature: %s, version: %d, flags_audio: %d, flags_video: %d, headersize: %d\n",
           TAB44, flvHeader.signature, flvHeader.version, flvHeader.flags_audio, flvHeader.flags_video, flvHeader.headersize);
#endif
}

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
static void DealScriptTagData(unsigned char *const tagData, unsigned int tagDataSize)
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
static void DealVideoTagData(unsigned char *const tagData)
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

static void DealAudioTagData(unsigned char *const tagData)
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

static void DealTagData(unsigned char *const tagData, const int tagType, const unsigned int tagSize)
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

int main(int argc, char *argv[])
{
    int dataLen = 0;
    int previousTagSize = 0;

    FILE *fp = NULL;

    unsigned char *tagData = NULL;

    unsigned char flvHeaderData[MIN_FLV_HEADER_LEN + 1] = {0};
    unsigned char preTagSizeData[MAX_PRE_TAG_SIZE_LEN + 1] = {0};
    unsigned char tagHeaderData[MAX_TAG_HEADER_LEN + 1] = {0};

    T_FLV_TAG_HEADER tagHeader = {0};

    if (2 != argc)
    {
        printf("Usage: flvparse **.flv\n");

        return -1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("open file[%s] error!\n", argv[1]);

        return -1;
    }

    memset(flvHeaderData, 0x0, sizeof(flvHeaderData));

    dataLen = fread(flvHeaderData, 1, MIN_FLV_HEADER_LEN, fp);
    if (dataLen != MIN_FLV_HEADER_LEN)
    {
        printf("read flv header error!\n");

        fclose(fp);

        return -1;
    }

    flvHeaderData[MIN_FLV_HEADER_LEN] = '\0';

    DealFlvHeader(flvHeaderData);

#ifdef PRINTF_DEBUG
    printf("+FLV Body\n");
#endif

    while (1)
    {
        memset(preTagSizeData, 0x0, sizeof(preTagSizeData));

        dataLen = fread(preTagSizeData, 1, MAX_PRE_TAG_SIZE_LEN, fp);
        if (dataLen != MAX_PRE_TAG_SIZE_LEN)
        {
            break;
        }

        preTagSizeData[MAX_PRE_TAG_SIZE_LEN] = '\0';

#ifdef PRINTF_DEBUG
        printf("%spreviousTagSize: %d\n", TAB44, (preTagSizeData[0] << 24) | (preTagSizeData[1] << 16) | (preTagSizeData[2] << 8) | preTagSizeData[3]);
#endif

        memset(tagHeaderData, 0x0, sizeof(tagHeaderData));

        dataLen = fread(tagHeaderData, 1, MAX_TAG_HEADER_LEN, fp);
        if (dataLen != MAX_TAG_HEADER_LEN)
        {
            continue;
        }

        memset(&tagHeader, 0x0, sizeof(T_FLV_TAG_HEADER));

        DealTagHeader(tagHeaderData, &tagHeader);

        tagData = (unsigned char *)malloc(tagHeader.data_size);
        if (!tagData)
        {
            continue;
        }

        memset(tagData, 0x0, tagHeader.data_size);

        dataLen = fread(tagData, 1, tagHeader.data_size, fp);
        if (dataLen != tagHeader.data_size)
        {
            continue;
        }

        DealTagData(tagData, tagHeader.type, tagHeader.data_size);

        free(tagData);
        tagData = NULL;
    }

    fclose(fp);

    return 0;
}