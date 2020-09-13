#include <iostream>
#include "AV_FFMPEG/AV_FFMPEG.h"
#include "AV_Venc/AV_Venc.h"
#include "CTime.hpp"
using namespace std;

// 测试编码USB获取YUV422
void endocer(const char *dev)
{
	AVPacket *pkt = NULL;
	AVFrame *frame = NULL;
	AV_FFMPEG *av_ffmpeg = NULL;
	AV_Venc *av_venc = NULL;
	CTimer timer;

	const char *codecfile = "/mnt/hgfs/shared/out.h264";

	av_ffmpeg = new AV_FFMPEG();
	av_venc = new AV_Venc();

#ifdef DEBUD
	const char *file = "/mnt/hgfs/shared/out.yuv";
	FILE *outflie = fopen(file, "wb+");
#endif
	FILE *outH264 = fopen(codecfile, "wb+");

	try
	{
		// 打开usb设备
		av_ffmpeg->Open(dev, "v4l2");
		av_venc->Open(av_ffmpeg->VideoWidth(), av_ffmpeg->VideoHeight(), "libx264", true);

		while (true)
		{
			// 获取pkt数据包
			pkt = av_ffmpeg->PacketData();
			if (!pkt)
				break;
				
#ifdef DEBUD
			timer.start();
			frame = av_venc->Yuyv422Pkt2Yuv420P(pkt); // 转YUV420P
			fwrite(frame->data[0], 307200, 1, outflie);
			fwrite(frame->data[1], 307200 / 4, 1, outflie);
			fwrite(frame->data[2], 307200 / 4, 1, outflie);
#endif
			pkt = av_venc->Encoder(pkt);
			if (pkt)
			{
				fwrite(pkt->data, pkt->size, 1, outH264);
			}

#ifdef DEBUD
			printf("time = %0.3fms\n", timer.end() / 1000);
#endif
			av_ffmpeg->freePacket();
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}

	frame = NULL;
	while (true)
	{
		pkt = av_venc->Encoder(frame);
		if (!pkt)
			break;
		printf("buffer ... \n");
		fwrite(pkt->data, pkt->size, 1, outH264);
	}
	
	av_venc->Close();
	av_ffmpeg->Close();
#ifdef DEBUD
	fclose(outflie);
#endif
	fclose(outH264);
}