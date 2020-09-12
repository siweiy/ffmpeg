#include <iostream>
#include "AV_FFMPEG/AV_FFMPEG.h"
#include "AV_FrameConvert/AV_FrameConvert.h"
using namespace std;

// 测试编码USB获取YUV422
void endocer(const char *dev)
{
	AVPacket *pkt = NULL;
	AV_FFMPEG *av_ffmpeg = NULL;
	AV_FrameConvert *av_convert = NULL;


	av_ffmpeg  = new AV_FFMPEG();
	av_convert = new AV_FrameConvert();

	try
	{
		// 打开usb设备
		av_ffmpeg->Open(dev, "v4l2");
		// av_convert->Open(av_ffmpeg->GetAVFormatContext()->streams[]);
		
		while (true)
		{
			// 获取pkt数据包
			pkt = av_ffmpeg->GetPacketData();
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
	}
}