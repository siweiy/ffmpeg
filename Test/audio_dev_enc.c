#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavcodec/codec.h"

uint8_t **src_data = NULL;
int src_linesize = 0;

uint8_t **dst_data = NULL;
int dst_linesize = 0;

static SwrContext *create_swr()
{
	SwrContext *swr_ctx = swr_alloc_set_opts(NULL,				  // 上下文
											 AV_CH_LAYOUT_STEREO, //输出channel布局
											 AV_SAMPLE_FMT_S16,	  //输出的采样格式
											 44100,				  // 采样率
											 AV_CH_LAYOUT_STEREO, //输入channel布局
											 AV_SAMPLE_FMT_FLT,	  //输入的采样格式
											 44100,				  //输入的采样率
											 0,
											 NULL);
	if (!swr_ctx)
	{
		exit(0);
	}

	if (swr_init(swr_ctx) < 0)
	{
		exit(0);
	}

	return swr_ctx;
}

void init_arr()
{
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
}

AVCodecContext *open_coder()
{
	// 找编码器
	// avcodec_find_decoder(AV_CODEC_ID_AAC);
	AVCodec *codec = avcodec_find_decoder_by_name("libfdk_aac");
	if (!codec)
	{
		printf("not find codec ...\n");
		return NULL;
	}

	// 创建上下文
	AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);

	codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
	codec_ctx->channels = 2;
	codec_ctx->sample_rate = 44100;
	// codec_ctx->bit_rate = 64000;// AAC_LC:128K, AAC HE:64K, AAC HE V2:32K
	codec_ctx->bit_rate = 0; // 设置0后，下一行才会生效
	codec_ctx->profile = FF_PROFILE_AAC_HE_V2;

	if (avcodec_open2(codec_ctx, codec, NULL) < 0)
	{
		printf("avcodec_open2 error .\n");
		exit(1);
	}

	return codec_ctx;
}

void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output)
{
	int ret = avcodec_send_frame(ctx, frame);

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(ctx, pkt);
		if (ret < 0)
		{
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				return;
			}
			else if(ret < 0)
			{
				printf("error, encoding .\n");
				exit(-1);
			}
		}
	}

	fwrite(pkt->data, 1, pkt->size, output);
	printf("pkt->size = %d\n", pkt->size);
	fflush(output);

	return;
}

// ffplay -ar 44100 -ac 2 -f f32le audio.pcm
int main(int argc, char const *argv[])
{
	const char *file_out = "audio.aac";
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

	AVCodecContext *codec_ctx = open_coder();

	AVFrame *frame = av_frame_alloc();
	if (!frame)
	{
		exit(0);
	}
	frame->nb_samples = 512; // 单通道一个音频帧的采样数
	frame->format = AV_SAMPLE_FMT_S16;
	frame->channel_layout = AV_CH_LAYOUT_STEREO;
	av_frame_get_buffer(frame, 0);

	AVPacket *newpkt = av_packet_alloc();
	if (!newpkt)
	{
		exit(0);
	}

	// ***************
	SwrContext *swr_ctx = create_swr();
	init_arr();

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

		// 将重采样数据拷贝到frmae
		memcpy((void *)frame->data[0], (void *)dst_data[0], dst_linesize);

		encode(codec_ctx, frame, newpkt, file);

		av_packet_unref(&pkt);
	}

	// 将缓冲区编码输出
	encode(codec_ctx, NULL, newpkt, file);

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

	av_frame_free(&frame);
	av_packet_unref(newpkt);

	return 0;
}
