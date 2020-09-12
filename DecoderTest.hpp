#pragma once

#include <unistd.h>
#include "AV_FFMPEG/AV_FFMPEG.h"
#include "AV_Vdec/AV_Vdec.h"
#include "AV_MyFmtConver/AV_ImgFmtConver.h"
#include "AV_FrameConvert/AV_FrameConvert.h"

#define TWO_PROGRAM 1

static FILE *voutfile = NULL;
static FILE *aoutfile = NULL;

void usemsg()
{
    printf("param[1]: rtsp address or device\n");
    printf("param[2]: output video file\n");
    printf("param[3]: video format, only use dev\n");

    exit(0);
}

// 写文件
void writePacket(AV_FFMPEG *av_ffmpeg)
{
    while (true)
    {
        if (TWO_PROGRAM)
        {
            AVPacket *pkt;
            pkt = av_ffmpeg->GetPacketData();
            if (pkt == NULL)
            {
                printf("pkt is null:1\n");
                av_ffmpeg->freePacket();
                break;
            }
            fwrite(pkt->data, pkt->size, 1, voutfile);
            fflush(voutfile);
            printf("pkt pos = %0.2f\n", av_ffmpeg->playPosition()); // 播放视频位置
        }
        else
        {
            AV_PACKET_DATA pkt;
            if (av_ffmpeg->GetPacketData(&pkt) < 0)
            {
                printf("pkt is null:2\n");
                av_ffmpeg->freePacket();
                break;
            }
            if (pkt.data_type == AV_TYPE_VIDEO)
                fwrite(pkt.packet_data, pkt.packet_size, 1, voutfile);
        }

        av_ffmpeg->freePacket();
    }

__EXIT:
    av_ffmpeg->freePacket();
}

// 打开文件、RTSP、dev三种测试
int rtsp_or_dev_test(int argc, const char *argv[])
{
    if (argc < 3)
    {
        usemsg();
    }

    AV_FFMPEG *av_ffmpeg = new AV_FFMPEG();

    try
    {
        if (argc == 3)
        {
            voutfile = fopen(argv[2], "wb+");
            if (av_ffmpeg->Open(argv[1]))
                throw - 1;
        }
        else
        {
            voutfile = fopen(argv[3], "wb+");
            if (av_ffmpeg->Open(argv[1], argv[2]))
                throw - 1;
        }

        printf("fps:%0.2f, duration = %0.2fs\n", av_ffmpeg->fps(), av_ffmpeg->durationSec()); // 视频长度

        writePacket(av_ffmpeg);
    }
    catch (...)
    {
        printf("have other execption \n");
    }

    fclose(voutfile);
    av_ffmpeg->Close();

    return 0;
}

// 解码、转换格式测试
int decoder_test(int argc, const char *argv[])
{
    if (argc < 2)
        usemsg();

    // 初始化
    int video_index;
    int audio_index;
    int subtitle_index;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    AV_FFMPEG *av_ffmpeg = new AV_FFMPEG();
    AV_Vdec *av_vdec = new AV_Vdec();
    AV_Vdec *av_adec = new AV_Vdec();
    // aoutfile = fopen("out.AAC", "wb+");
    voutfile = fopen("/mnt/hgfs/shared/out.yuv420p", "wb+");
    try
    {
        if (av_ffmpeg->Open(argv[1], false) < 0)
            throw - 1;

        printf("all time = %0.2f\n", av_ffmpeg->durationSec());

        video_index = av_ffmpeg->GetVideoIndex();
        audio_index = av_ffmpeg->GetAudioIndex();
        subtitle_index = av_ffmpeg->GetSubtitleIndex();

        // 设置参数 -- 视频
        av_vdec->setFmtCtxAndIndex(av_ffmpeg->GetAVFormatContext(), video_index);
        if (av_vdec->Open() < 0)
            throw - 3;

        // 设置参数 -- 音频
        if (av_ffmpeg->GetAudioIndex() >= 0)
        {
            av_adec->setFmtCtxAndIndex(av_ffmpeg->GetAVFormatContext(), audio_index);
            if (av_adec->Open() < 0)
                throw - 4;
        }

        AV_FrameConvert *av_FrameConvert = new AV_FrameConvert(av_vdec->getAVCodecContext());
        av_FrameConvert->Open(AV_PIX_FMT_YUV420P, SWS_BICUBIC);
        while (true)
        {
            int index;
            // 获取数据包
            pkt = av_ffmpeg->GetPacketData(index);
            if (pkt == NULL)
            {
                printf("pkt is null:1\n");
                av_ffmpeg->freePacket();
                break;
            }

            printf("time = %.2f\n", av_ffmpeg->playPosition());

            if (index == video_index)
            {
                // 视频解码
                frame = av_vdec->Decoder(pkt);
                if (!frame)
                {
                    av_ffmpeg->freePacket();
                    continue;
                }

                uint8_t *yuv420 = av_FrameConvert->transform(frame);
                if (yuv420)
                {
                    fwrite(yuv420, av_FrameConvert->getBufferSize(), 1, voutfile);
                    fflush(voutfile);
                }
            }
            else if (index == audio_index)
            {
                // 音频解码
                frame = av_adec->Decoder(pkt);
                if (!frame)
                {
                    av_ffmpeg->freePacket();
                    continue;
                }
            }
            else
            {
                printf("其他");
            }

            // 释放资源
            av_ffmpeg->freePacket();

            usleep(1000);
        }

        av_FrameConvert->Close();
    }
    catch (int ret)
    {
        printf("have other execption : ret = %d\n", ret);
    }
    av_vdec->Close();
    av_ffmpeg->Close();

    fclose(voutfile);

    return 0;
}
