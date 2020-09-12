#include "DecoderTest.hpp"
#include "EndocerTest.hpp"

// 测试代码 -- open rtsp or dev
int main(int argc, const char *argv[])
{
    // rtsp://admin:ematech123@172.16.98.223:554/h264/ch1/main/av_stream
    // rtsp://admin:ematech1234@172.16.100.167/ipc
    // rtsp://172.16.100.159/h264/ch1/main/av_stream
#if 0
    // 测试打开视频流
    rtsp_or_dev_test(argc, argv);
#else
    // 测试解码
    // decoder_test(argc, argv);
#endif
    // 测试编码
    endocer("/dev/video0");

    return 0;
}