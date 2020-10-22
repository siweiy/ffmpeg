#include "AV_Write/AV_Write.h"

namespace AVWrite_Test{
	#define in_url  "rtsp://192.168.16.86/h264/ch1/main/av_stream"
	#define out_url "rtmp://localhost:1935/live/test"

	void rtmp_test() {
		AV_Write av_write;
		av_write.Open(in_url, out_url);
		// av_write.Open(in_url, "out.mp4", "mp4");
		// av_write.Open(in_url, "out.ts", "ts");
		// av_write.Open(in_url, "out.flv", "flv");
		// av_write.Open(in_url, "out.avi", "avi");
		while (true) {
			av_write.Processing();
		}
		av_write.Close();
	}

};
