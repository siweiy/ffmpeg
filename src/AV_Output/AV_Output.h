#pragma once

extern "C" {
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libavutil/mathematics.h"
	#include "libavutil/time.h"
};

class AV_Output
{
public:
	AV_Output();
	~AV_Output();

private:
};
