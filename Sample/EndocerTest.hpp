#include <iostream>
#include "AV_FFMPEG/AV_FFMPEG.h"
#include "AV_Venc/AV_Venc.h"
#include "AV_RTMP/AV_RTMP.h"
#include "CTime.hpp"
using namespace std;

// #define DEBUD
// #define _RTMP_STREAM

// 测试编码USB获取YUV422
void endocer(const char *dev)
{
	AVPacket *pkt = NULL;
	AVFrame *frame = NULL;
	AV_FFMPEG *av_ffmpeg = NULL;
	AV_Venc *av_venc = NULL;
#ifdef _RTMP_STREAM
	AV_RTMP *av_rtmp = NULL;
#endif
	CTimer timer;

	av_ffmpeg = new AV_FFMPEG();
	av_venc = new AV_Venc();
#ifdef _RTMP_STREAM
	av_rtmp = new AV_RTMP();
#endif

#ifdef DEBUD
	const char *file = "/mnt/hgfs/share/out.yuv";
	const char *codecfile = "/mnt/hgfs/share/out.h264";
	FILE *outflie = fopen(file, "wb+");
	FILE *outH264 = fopen(codecfile, "wb+");
#endif

	try
	{
		// 打开usb设备
		av_ffmpeg->Open(dev, "video4linux2");
		av_venc->Open(av_ffmpeg->VideoWidth(), av_ffmpeg->VideoHeight(), "libx264", true);
#ifdef _RTMP_STREAM
		av_rtmp->Open("");
#endif
		int count = 1;
		while (true)
		{
#ifdef DEBUD
			timer.start();
#endif
			// 获取pkt数据包
			pkt = av_ffmpeg->PacketData();
			if (!pkt)
				break;

#ifdef DEBUD
			frame = av_venc->Yuyv422Pkt2Yuv420P(pkt, 307200); // 转YUV420P
			fwrite(frame->data[0], 307200, 1, outflie);
			fwrite(frame->data[1], 307200 / 4, 1, outflie);
			fwrite(frame->data[2], 307200 / 4, 1, outflie);
#endif
			pkt = av_venc->Encoder(pkt);
			if (pkt)
			{
#ifdef DEBUD
				printf("count = %d, time = %ldms\n", count++, timer.end() / 1000);
				fwrite(pkt->data, pkt->size, 1, outH264);
#endif

#ifdef _RTMP_STREAM // RTMP推流
				// av_rtmp->PushRtmp();
#endif
			}

			av_ffmpeg->freePacket();
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}

#if 0
	// 处理文件时使用,结束时将缓存区拿出来
	frame = NULL;
	while (true)
	{
		pkt = av_venc->Encoder(frame);
		if (!pkt)
			break;
		printf("buffer ... \n");
		fwrite(pkt->data, pkt->size, 1, outH264);
	}
#endif

	av_venc->Close();
	av_ffmpeg->Close();
#ifdef _RTMP_STREAM
	av_rtmp->Close();
#endif
#ifdef DEBUD
	fclose(outflie);
	fclose(outH264);
#endif
}