#pragma once

typedef unsigned char uint8_t;
typedef long int int64_t;

typedef enum AV_TYPE_
{
    AV_TYPE_AUDIO,   // 音频
    AV_TYPE_VIDEO,   // 视频
    AV_TYPE_SUBTITLE // 字幕
}AV_INDEX_TYPE;

typedef struct AV_PACKET_DATA_
{
    enum AV_TYPE_ data_type; // 数据类型
    uint8_t *packet_data;    // 数据
    int packet_size;         // 数据大小
    int64_t packet_pts;      // 显示时间
    bool key_frame;          // 关键帧
} AV_PACKET_DATA;