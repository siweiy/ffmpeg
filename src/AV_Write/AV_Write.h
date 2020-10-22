#pragma once

#include <iostream>
extern "C" {
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/mathematics.h"
	#include "libavutil/time.h"
};


class AV_Write
{
public:
	AV_Write();
	~AV_Write();

	/**
	 * @brief open url/file, get v/a stream to out_url
	 * @param in_url: input rtsp or file name
	 * @param out_url: output url or file name
	 * @param net_stream: identify network
	 * @param protocol: output stream is 'tcp' or 'udp' 
	 * @return open true or false
	*/
	bool Open(std::string in_url, std::string out_url, std::string protocol="tcp", bool net_stream=true);
	bool Processing();
	void Close();

private:
	bool CreateInAVFormatContext(std::string in_url, bool net_stream=true);
	bool CreateOutAVFormatContext(std::string out_url, std::string protocol);
	bool AVIOOpen(std::string out_url);
	bool AVIOWriteHeader();
	bool AVIOWriteTrailer();

	AVFormatContext *m_pInFormatCtx;
	AVFormatContext *m_pOutFormatCtx;
	AVOutputFormat *m_pOformat;
	AVPacket *m_pPacket;
	int m_nStream[5];// 0 video 1 audio 2 subtitle ...

};


