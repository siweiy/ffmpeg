#include "AV_Venc.h"

AV_Venc::AV_Venc()
{
}

AV_Venc::~AV_Venc()
{
}

bool AV_Venc::Open(int width, int height, const char *codec, bool needFrmae)
{
	// 找编码器
	AVCodec *pCodec = avcodec_find_decoder_by_name(codec);
	if (!pCodec)
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] avcodec_find_decoder_by_name decoder error \n", __func__, __LINE__);
		return false;
	}

	// 获取编码器上下文
	m_pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!m_pCodecCtx)
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] avcodec_alloc_context3 error \n", __func__, __LINE__);
		return false;
	}

	// SPS/PPS  编码最高标准
	m_pCodecCtx->profile = FF_PROFILE_H264_HIGH_444;
	m_pCodecCtx->level = 50; // 表示LEVEL是5.0

	// 设置分辨率
	m_pCodecCtx->width = width;
	m_pCodecCtx->height = height;

	// GOP
	// 如果I帧丢失，会出现花屏或者卡段现象
	m_pCodecCtx->gop_size = 250;
	// 最小插入I帧间隔
	m_pCodecCtx->keyint_min = 25; // 可选项

	// 设置B帧数据
	m_pCodecCtx->max_b_frames = 3; // 可选项
	// 解码器中帧重排序缓冲区的大小。
	m_pCodecCtx->has_b_frames = 1; // 可选项

	// 设置参考帧数量
	m_pCodecCtx->refs = 3; // 可选项

	// 设置输入YUV格式
	m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// 设置码率
	m_pCodecCtx->bit_rate = 600000; // 码流大小600kbps

	// 设置帧数
	m_pCodecCtx->time_base = (AVRational){1, 25}; // 帧与帧之间的间隔time_base
	m_pCodecCtx->framerate = (AVRational){25, 1}; // 帧率、每秒25帧

	if (avcodec_open2(m_pCodecCtx, pCodec, NULL))
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] avcodec_open2 error \n", __func__, __LINE__);
		return false;
	}

	return OnFramePacket(width, height, needFrmae);
}

void AV_Venc::Close()
{
	if (m_pCodecCtx)
		avcodec_free_context(&m_pCodecCtx);
	if (m_pFrame)
		av_frame_free(&m_pFrame);
	if (m_packet)
		av_packet_unref(m_packet);
}

AVPacket *AV_Venc::Encoder(AVFrame *pFrame)
{
	// 编码
	if (avcodec_send_frame(m_pCodecCtx, pFrame) < 0)
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] avcodec_send_frame error \n", __func__, __LINE__);
		return NULL;
	}

	if (avcodec_receive_packet(m_pCodecCtx, m_packet) < 0)
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] avcodec_receive_packet error \n", __func__, __LINE__);
		return NULL;
	}

	return m_packet;
}

AVPacket *AV_Venc::Encoder(AVPacket *pkt)
{
	// 将 pkt 转为 m_pFrame YUV420P 数据
	Yuyv422Pkt2Yuv420P(pkt);

	// 编码
	if (avcodec_send_frame(m_pCodecCtx, m_pFrame) < 0)
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] avcodec_send_frame error \n", __func__, __LINE__);
		return NULL;
	}

	if (avcodec_receive_packet(m_pCodecCtx, m_packet) < 0)
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] avcodec_receive_packet error \n", __func__, __LINE__);
		return NULL;
	}

	return m_packet;
}

bool AV_Venc::OnFramePacket(int width, int height, bool frame)
{
	if (frame)
	{
		m_pFrame = av_frame_alloc();
		if (!m_pFrame)
		{
			av_log(NULL, AV_LOG_INFO, "[ %s : %d ] av_frame_alloc error \n", __func__, __LINE__);
			return false;
		}

		m_pFrame->width = width;
		m_pFrame->height = height;
		m_pFrame->format = AV_PIX_FMT_YUV420P;

		// 以32字节对齐
		if (av_frame_get_buffer(m_pFrame, 32) < 0)
		{
			av_log(NULL, AV_LOG_INFO, "[ %s : %d ] av_frame_get_buffer error \n", __func__, __LINE__);
			goto __ERROR;
		}
	}

	m_packet = av_packet_alloc();
	if (!m_packet)
	{
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] av_packet_alloc error \n", __func__, __LINE__);
		goto __ERROR;
	}

	return true;

__ERROR:
	if (m_pFrame)
		av_frame_free(&m_pFrame);

	return false;
}

AVFrame *AV_Venc::Yuyv422Pkt2Yuv420P(AVPacket *pkt)
{
	// YUYV422 --------- YUYV YUVY YUYV YUYV ----- 
	for (int i = 0; i < 307200 / 4; i++)
	{
		m_pFrame->data[0][i * 4] = pkt->data[i * 8];		 // Y
		m_pFrame->data[1][i] = pkt->data[i * 8 + 1];		 // U
		m_pFrame->data[0][i * 4 + 1] = pkt->data[i * 8 + 2]; // Y
		m_pFrame->data[2][i] = pkt->data[i * 8 + 3];		 // V

		// 丢弃偶数UV
		m_pFrame->data[0][i * 4 + 2] = pkt->data[i * 8 + 4]; // Y
		m_pFrame->data[0][i * 4 + 3] = pkt->data[i * 8 + 6]; // Y
	}

	return m_pFrame;
}