#include "AV_FrameConvert.h"

AV_FrameConvert::AV_FrameConvert(AVCodecContext *codec_ctx) : m_pCodecCtx(codec_ctx)
{
}

AV_FrameConvert::~AV_FrameConvert()
{
    if (m_pFrameSws)
        av_frame_free(&m_pFrameSws);
}

bool AV_FrameConvert::Open(enum AVPixelFormat sws_fmt, int sws_algorithn)
{
    m_pFrameSws = av_frame_alloc();
    if (!m_pFrameSws)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]alloc frame error: pFrameSws\n", __func__, __LINE__);
        return false;
    }

    // 将解码后的YUV数据转换成xxx, 得到转换格式上下文
    m_imgConvertCtx = sws_getContext(m_pCodecCtx->width,   // 输入源宽
                                     m_pCodecCtx->height,  // 输入源高
                                     m_pCodecCtx->pix_fmt, // 输入源格式
                                     m_pCodecCtx->width,   // 输出源宽
                                     m_pCodecCtx->height,  // 输出源高
                                     sws_fmt,              // 输出为RGB32格式数据
                                     sws_algorithn,        // 使用算法
                                     NULL,                 // 输入源过滤器
                                     NULL,                 // 输出源过滤器
                                     NULL                  // 过滤器参数
    );
    if (!m_imgConvertCtx)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]sws_getContext error \n", __func__, __LINE__);
        return false;
    }

#if 1
    m_outBufferSize = av_image_get_buffer_size(sws_fmt, m_pCodecCtx->width, m_pCodecCtx->height, 1);

    // 分配空间
    m_outBuffer = (uint8_t *)av_malloc(m_outBufferSize * sizeof(uint8_t));

    // 最终数据填充到 m_outBuffer
    if (av_image_fill_arrays(m_pFrameSws->data, m_pFrameSws->linesize, m_outBuffer, sws_fmt, m_pCodecCtx->width, m_pCodecCtx->height, 1) < 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]av_image_fill_arrays error \n", __func__, __LINE__);
        return false;
    }
#else

    // 获取交换格式一帧大小
    int numBytes = avpicture_get_size(sws_fmt, m_pCodecCtx->width, m_pCodecCtx->height);

    // 分配空间
    m_outBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    // 将数据fill到out_buffer， 每次解码后数据都会在out_buffer
    if (avpicture_fill((AVPicture *)m_pFrameSws, m_outBuffer, sws_fmt, m_pCodecCtx->width, m_pCodecCtx->height) < 0)
    {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]avpicture_fill error \n", __func__, __LINE__);
        return false;
    }

#endif
}

uint8_t *AV_FrameConvert::transform(AVFrame *pFrame)
{
    sws_scale(m_imgConvertCtx,                      // 图像格式上下文
              (const uint8_t *const *)pFrame->data, // 原数据
              pFrame->linesize,                     // 源数据linesize
              0,                                    // slice起始位置
              m_pCodecCtx->height,                  // slice高度
              m_pFrameSws->data,                    // dst数据
              m_pFrameSws->linesize);               // dst linesize

    return m_outBuffer;
}

int AV_FrameConvert::getBufferSize()
{
    return m_outBufferSize;
}

void AV_FrameConvert::Close()
{
    if (m_pFrameSws)
        av_frame_free(&m_pFrameSws);
    if (m_outBuffer)
        av_free(m_outBuffer);
    if (m_imgConvertCtx)
        sws_freeContext(m_imgConvertCtx);
}