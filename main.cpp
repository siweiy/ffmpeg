#include "DecoderTest.hpp"
#include "EndocerTest.hpp"
#include "Flv2RtmpTest.hpp"
#include "H264ToRtmp.hpp"
#include "AVWriteClassTest.hpp"
using namespace AVWrite_Test;

// 测试代码 -- open rtsp or dev
int main(int argc, const char *argv[])
{
    // rtsp://admin:ematech123@172.16.98.223:554/h264/ch1/main/av_stream
    // rtsp://admin:ematech1234@172.16.100.167/ipc
    // rtsp://172.16.100.159/h264/ch1/main/av_stream
    // rtsp://192.168.16.86/h264/ch1/main/av_stream
    // rtmp://localhost:1935/live/test

    // 测试打开视频流
    // rtsp_or_dev_test(argc, argv);

    // 测试解码
    // decoder_test(argc, argv);

    // 测试编码H264 -- H264直接推流RTMP
    // endocer("/dev/video0");

    // 读取FLV推流RTMP
    // flv_rtmp_test1();
    // flv_rtmp_test2();

    // 推H264流
    // h264ToRtmp();

    // AVWrite 类测试
    AVWrite_Test::rtmp_test();

    return 0;
}