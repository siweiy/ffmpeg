#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"

// ffplay -ar 44100 -ac 2 -f f32le audio.pcm
int main(int argc, char const *argv[])
{
	const char *file_out = "audio.pcm";
	const char *audio_dev = "hw:0,0";
	FILE *file = NULL;
	AVPacket pkt;
	AVFormatContext *fmt_ctx = NULL;
	AVInputFormat *input_ftx = NULL;
	AVDictionary *option = NULL;

	avdevice_register_all();
	av_log_set_level(AV_LOG_DEBUG);

	input_ftx = av_find_input_format("alsa");
	assert(input_ftx != NULL);

	assert(avformat_open_input(&fmt_ctx, audio_dev, input_ftx, &option) == 0);

	av_dump_format(fmt_ctx, 0, audio_dev, 0);

	file = fopen(file_out, "wb+");

	// ***************
	SwrContext *swr_ctx = NULL;

	swr_ctx = swr_alloc_set_opts(NULL,				  // 上下文
								 AV_CH_LAYOUT_STEREO, //输出channel布局
								 AV_SAMPLE_FMT_S16,	  //输出的采样格式
								 44100,				  // 采样率
								 AV_CH_LAYOUT_STEREO, //输入channel布局
								 AV_SAMPLE_FMT_FLT,	  //输入的采样格式
								 48000,				  //输入的采样率
								 0,
								 NULL);
	if (!swr_ctx)
	{
		return -1;
	}

	if (swr_init(swr_ctx) < 0)
	{
		return -1;
	}

	uint8_t **src_data = NULL;
	int src_linesize = 0;

	uint8_t **dst_data = NULL;
	int dst_linesize = 0;

	// 64/4=16/2=8
	// 创建输入缓冲区
	av_samples_alloc_array_and_samples(&src_data,		  //输入参数
									   &src_linesize,	  //缓冲区大小
									   2,				  //通道个数
									   512,				  //单通道采样个数
									   AV_SAMPLE_FMT_FLT, //采样格式
									   0);				  //对齐

	av_samples_alloc_array_and_samples(&dst_data,		  //输出参数
									   &dst_linesize,	  //缓冲区大小
									   2,				  //通道个数
									   512,				  //单通道采样个数
									   AV_SAMPLE_FMT_S16, //采样格式
									   0);
	int ret = 0;
	av_init_packet(&pkt);
	while (1)
	{
		if (av_read_frame(fmt_ctx, &pkt) < 0)
			break;

		memcpy((void *)src_data[0], (void *)pkt.data, pkt.size);
		// 重采样
		ret = swr_convert(swr_ctx,					  //上下文
						  dst_data,					  //输出结果缓冲区
						  512,						  //每个通道的采样数
						  (const uint8_t **)src_data, //输入缓冲区
						  512						  //输入单个通道的采样数
		);
		if (ret < 0)
		{
			break;
		}

		printf("pkt.size = %d\n", pkt.size);
		// fwrite(pkt.data, pkt.size, 1, file);
		printf("dst_linesize = %d\n", dst_linesize);
		fwrite(dst_data[0], 1, dst_linesize, file);
		fflush(file);

		av_packet_unref(&pkt);
	}

	avformat_close_input(&fmt_ctx);
	fclose(file);

	if (src_data)
	{
		av_freep(&src_data[0]);
	}
	av_freep(src_data);

	if (dst_data)
	{
		av_freep(&dst_data[0]);
	}
	av_freep(dst_data);

	swr_free(&swr_ctx);

	return 0;
}
