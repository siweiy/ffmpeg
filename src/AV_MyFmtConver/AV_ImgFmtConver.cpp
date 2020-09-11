#include "AV_ImgFmtConver.h"

AV_ImgFmtConver *AV_ImgFmtConver::m_ImgFmtConver = NULL;

AV_ImgFmtConver::AV_ImgFmtConver()
{
}

AV_ImgFmtConver::~AV_ImgFmtConver()
{
}

AV_ImgFmtConver *AV_ImgFmtConver::GetInstall()
{
    if (!m_ImgFmtConver)
        m_ImgFmtConver = new AV_ImgFmtConver();

    return m_ImgFmtConver;
}
void AV_ImgFmtConver::Release()
{
    if (m_ImgFmtConver)
    {
        delete m_ImgFmtConver;
        m_ImgFmtConver = NULL;
    }
}

uint8_t *AV_ImgFmtConver::AVFrame2YUV(AVFrame *pFrame)
{
    switch (pFrame->format)
    {
    case AV_PIX_FMT_YUV420P:
    {
        return AVFrame2YUV420P(pFrame);
    }
    break;
    case AV_PIX_FMT_YUV422P:
    {
    }
    break;
    case AV_PIX_FMT_YUYV422:
    {
    }
    break;
    case AV_PIX_FMT_RGB32:
    {
    }
    break;
    default:
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]fmt:%d\n", __func__, __LINE__, pFrame->format);
        break;
    }

    return NULL;
}

uint8_t *AV_ImgFmtConver::AVFrame2YUV420P(AVFrame *pFrame)
{
    int frameHeight = pFrame->height;
    int frameWidth = pFrame->width;
    int channels = 3;

    // 反转图像
    pFrame->data[0] += pFrame->linesize[0] * (frameHeight - 1);
    pFrame->linesize[0] *= -1;
    pFrame->data[1] += pFrame->linesize[1] * (frameHeight / 2 - 1);
    pFrame->linesize[1] *= -1;
    pFrame->data[2] += pFrame->linesize[2] * (frameHeight / 2 - 1);
    pFrame->linesize[2] *= -1;

    //创建保存yuv数据的buffer
    uint8_t *pDecodedBuffer = (uint8_t *)malloc(
        frameHeight * frameWidth * sizeof(uint8_t) * channels / 2);

    //从AVFrame中获取yuv420p数据，并保存到buffer
    int i, j, k;
    //拷贝y分量
    for (i = 0; i < frameHeight; i++)
    {
        memcpy(pDecodedBuffer + frameWidth * i,
               pFrame->data[0] + pFrame->linesize[0] * i,
               frameWidth);
    }
    //拷贝u分量
    for (j = 0; j < frameHeight / 2; j++)
    {
        memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j,
               pFrame->data[1] + pFrame->linesize[1] * j,
               frameWidth / 2);
    }
    //拷贝v分量
    for (k = 0; k < frameHeight / 2; k++)
    {
        memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j + frameWidth / 2 * k,
               pFrame->data[2] + pFrame->linesize[2] * k,
               frameWidth / 2);
    }
    return pDecodedBuffer;
}

uint8_t *AV_ImgFmtConver::AVFrame2YUV420(AVFrame *pFrame)
{
    int frameHeight = pFrame->height;
    int frameWidth = pFrame->width;
    int channels = 3;

    //创建保存yuv数据的buffer
    uint8_t *pDecodedBuffer = (uint8_t *)malloc(
        frameHeight * frameWidth * sizeof(uint8_t) * channels);

    //从AVFrame中获取yuv420p数据，并保存到buffer
    int i, j, k;
    //拷贝y分量
    for (i = 0; i < frameHeight; i++)
    {
        memcpy(pDecodedBuffer + frameWidth * i,
               pFrame->data[0] + pFrame->linesize[0] * i,
               frameWidth);
    }
    //拷贝u分量
    for (j = 0; j < frameHeight / 2; j++)
    {
        memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j,
               pFrame->data[2] + pFrame->linesize[2] * j,
               frameWidth / 2);
    }
    //拷贝v分量
    for (k = 0; k < frameHeight / 2; k++)
    {
        memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j + frameWidth / 2 * k,
               pFrame->data[1] + pFrame->linesize[1] * k,
               frameWidth / 2);
    }

    return pDecodedBuffer;
}

// AVFrame 转 YUV
uint8_t *AV_ImgFmtConver::AVFrame2YUV422P(AVFrame *pFrame)
{
}

uint8_t *AV_ImgFmtConver::AVFrame2YUV422(AVFrame *pFrame)
{
}
uint8_t *AV_ImgFmtConver::AVFrame2RGB24(AVFrame *pFrame)
{

}

uint8_t* AV_ImgFmtConver::YUV420P2RGB24(const uint8_t *yuv_buffer, int width, int height)
{
    int channels = 3;

    uint8_t *rgb_buffer = (uint8_t *)malloc(
        height * width * sizeof(uint8_t) * channels);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index_Y = y * width + x;
            int index_U = width * height + y / 2 * width / 2 + x / 2;
            int index_V = width * height + width * height / 4 + y / 2 * width / 2 + x / 2;

            // 取出 YUV
            uint8_t Y = yuv_buffer[index_Y];
            uint8_t U = yuv_buffer[index_U];
            uint8_t V = yuv_buffer[index_V];

            // YCbCr420
            int R = Y + 1.402 * (V - 128);
            int G = Y - 0.34413 * (U - 128) - 0.71414 * (V - 128);
            int B = Y + 1.772 * (U - 128);

            // 确保取值范围在 0 - 255 中
            R = (R < 0) ? 0 : R;
            G = (G < 0) ? 0 : G;
            B = (B < 0) ? 0 : B;
            R = (R > 255) ? 255 : R;
            G = (G > 255) ? 255 : G;
            B = (B > 255) ? 255 : B;

            rgb_buffer[(y * width + x) * channels + 2] = uint8_t(R);
            rgb_buffer[(y * width + x) * channels + 1] = uint8_t(G);
            rgb_buffer[(y * width + x) * channels + 0] = uint8_t(B);
        }
    }

    return rgb_buffer;
}