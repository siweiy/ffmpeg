#include "AV_Write.h"

AV_Write::AV_Write() : 
	m_pOutFormatCtx(NULL), m_pInFormatCtx(NULL), \
	m_pOformat(NULL), m_pPacket(NULL) {

	avformat_network_init();
	memset(m_nStream, -1, sizeof(m_nStream));
}

AV_Write::~AV_Write() { }

bool AV_Write::CreateInAVFormatContext(std::string in_url, bool net_stream) {
	
	int ret = 0;
	AVDictionary *options = NULL;

	m_pInFormatCtx = avformat_alloc_context();
    if (!m_pInFormatCtx) {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]alloc context error\n", __func__, __LINE__);
        return false;
    }
	
    m_pPacket = av_packet_alloc();
    if (!m_pPacket) {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]alloc packet error\n", __func__, __LINE__);
        return false;
    }
	
    if (net_stream) {
        av_dict_set(&options, "buffer_size", "4096000", 0);
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
        av_dict_set(&options, "stimeout", "5000000", 0);
        av_dict_set(&options, "max_delay", "500000", 0);
        // av_dict_set(&options, "framerate", "25", 0);
        av_dict_set(&options, "flags", "nobuffer", 0);
    }

	if ((ret = avformat_open_input(&m_pInFormatCtx, in_url.c_str(), 0, &options)) < 0) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Could not open : %s\n", __func__, __LINE__, in_url.c_str());
		return false;
	}

	if ((ret = avformat_find_stream_info(m_pInFormatCtx, 0)) < 0) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Failed to retrieve input stream information.\n", __func__, __LINE__);
		return false;
	}

	m_nStream[0] = av_find_best_stream(m_pInFormatCtx, AVMEDIA_TYPE_VIDEO, 0, 0, NULL, 0);
	m_nStream[1] = av_find_best_stream(m_pInFormatCtx, AVMEDIA_TYPE_AUDIO, 0, 0, NULL, 0);
	m_nStream[2] = av_find_best_stream(m_pInFormatCtx, AVMEDIA_TYPE_SUBTITLE, 0, 0, NULL, 0);

	if (m_nStream[0] < 0) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ]can't find input video stream.\n", __func__, __LINE__);
		return false;
	}

	av_dump_format(m_pInFormatCtx, 0, in_url.c_str(), 0);

	return true;
}

const char* AnalysisOutputFormat(std::string protocol) {
	// std::string fmt = protocol.substr(protocol.find_last_of(".")+1, -1);
	// std::cout << fmt << " ------------------ " << std::endl;
	if ( !protocol.compare("mp4") || !protocol.compare("ts") ) { return "mpegts"; }
	return protocol.c_str();
}

bool AV_Write::CreateOutAVFormatContext(std::string out_url, std::string protocol) {

	int i, ret;
	if ( !protocol.compare("tcp") )
    	avformat_alloc_output_context2(&m_pOutFormatCtx, NULL, "flv", out_url.c_str()); //TCP
	else if ( !protocol.compare("udp") ) {
		avformat_alloc_output_context2(&m_pOutFormatCtx, NULL, "mpegts", out_url.c_str());//UDP --- ts,rtp server
	} else {	// if not tcp/udp
		avformat_alloc_output_context2(&m_pOutFormatCtx, NULL, AnalysisOutputFormat(protocol), out_url.c_str());//output file
	}

    if (!m_pOutFormatCtx) {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]can't create output video FormatCtx.\n", __func__, __LINE__);
		return false;
    }

    m_pOformat = m_pOutFormatCtx->oformat;
    for (i = 0; i < m_pInFormatCtx->nb_streams; i++) 
	{
        //根据输入流创建输出流（Create output AVStream according to input AVStream）
        AVStream *in_stream = m_pInFormatCtx->streams[i];
        AVStream *out_stream = avformat_new_stream(m_pOutFormatCtx, NULL);
		if (!out_stream) {
			av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Failed allocating output stream.\n", __func__, __LINE__);
			return false;
		}
 
		ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		if (ret < 0) {
			av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Failed avcodec_parameters_copy.\n", __func__, __LINE__);
			return false;
		}

		out_stream->codecpar->codec_tag = 0;
    }

    av_dump_format(m_pOutFormatCtx, 0, out_url.c_str(), 1);

	return true;
}

bool AV_Write::AVIOOpen(std::string out_url) {

	int ret;
    if (m_pOformat->flags & AVFMT_NOFILE) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ]AVFMT_NOFILE '%s'.\n", __func__, __LINE__, out_url.c_str());
		return false;
    }

	ret = avio_open(&m_pOutFormatCtx->pb, out_url.c_str(), AVIO_FLAG_WRITE);
	if (ret < 0) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Could not open output URL '%s'.\n", __func__, __LINE__, out_url.c_str());
		return false;
	}

	return true;
}

bool AV_Write::AVIOWriteHeader() {

	int ret;
    ret = avformat_write_header(m_pOutFormatCtx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Error avformat_write_header output URL.\n", __func__, __LINE__);
		return false;
    }

	return true;
}

bool AV_Write::AVIOWriteTrailer() {

	int ret;
    ret = av_write_trailer(m_pOutFormatCtx);
	if (ret < 0) {
        av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Error av_write_trailer output URL.\n", __func__, __LINE__);
		return false;
    }

	return true;
}

bool AV_Write::Open(std::string in_url, std::string out_url, std::string protocol, bool net_stream) {

	if ( !CreateInAVFormatContext(in_url, net_stream) ) { return false; }
	if ( !CreateOutAVFormatContext(out_url, protocol) ) { return false; }
	if ( !AVIOOpen(out_url) ) { return false; }
	if ( !AVIOWriteHeader() ) { return false; }

	return true;
}

bool AV_Write::Processing()
{
    static int64_t start_time = av_gettime();
	static uint64_t frame_index = 0;

	int ret;
	ret = av_read_frame(m_pInFormatCtx, m_pPacket);
	if (ret < 0) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ]Error av_read_frame.\n", __func__, __LINE__);
		return false;
	}
	
	// FIX：No PTS (Example: Raw H.264)
	// Write PTS
	if (m_pPacket->pts == AV_NOPTS_VALUE)
	{
		// Write PTS
		AVRational time_base1 = m_pInFormatCtx->streams[m_nStream[0]]->time_base;
		// Duration between 2 frames (us)
		int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(m_pInFormatCtx->streams[m_nStream[0]]->r_frame_rate);
		// Parameters
		m_pPacket->pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
		m_pPacket->dts = m_pPacket->pts;
		m_pPacket->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
	}

	//Important:Delay
	if (m_pPacket->stream_index == m_nStream[0])
	{
		AVRational time_base = m_pInFormatCtx->streams[m_nStream[0]]->time_base;
		AVRational time_base_q = {1, AV_TIME_BASE};
		int64_t pts_time = av_rescale_q(m_pPacket->dts, time_base, time_base_q);
		int64_t now_time = av_gettime() - start_time;
		if (pts_time > now_time)
			av_usleep(pts_time - now_time);
	}

	AVStream *in_stream = m_pInFormatCtx->streams[m_pPacket->stream_index];
	AVStream *out_stream = m_pOutFormatCtx->streams[m_pPacket->stream_index];

	/* copy packet */
	//转换PTS/DTS（Convert PTS/DTS）
	m_pPacket->pts = av_rescale_q_rnd(m_pPacket->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	m_pPacket->dts = av_rescale_q_rnd(m_pPacket->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	m_pPacket->duration = av_rescale_q(m_pPacket->duration, in_stream->time_base, out_stream->time_base);
	m_pPacket->pos = -1;

#ifdef DEBUG
	// Print to Screen
	if (m_pPacket->stream_index == m_nStream[0]) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] Send %10ld video frames to output URL.\n", __func__, __LINE__, frame_index);
	}
#endif

	frame_index++;

	ret = av_interleaved_write_frame(m_pOutFormatCtx, m_pPacket);
	if (ret < 0) {
		av_log(NULL, AV_LOG_INFO, "[ %s : %d ] Error muxing packet.\n", __func__, __LINE__);
		av_packet_unref(m_pPacket);
		return false;
	}

	av_packet_unref(m_pPacket);

	return true;
}

void AV_Write::Close() {

	AVIOWriteTrailer();

    if (m_pOutFormatCtx && !(m_pOformat->flags & AVFMT_NOFILE))
        avio_close(m_pOutFormatCtx->pb);
	if (m_pInFormatCtx) avformat_close_input(&m_pInFormatCtx);
	if (m_pOutFormatCtx) avformat_free_context(m_pOutFormatCtx);
	if (m_pPacket) av_packet_unref(m_pPacket);
}