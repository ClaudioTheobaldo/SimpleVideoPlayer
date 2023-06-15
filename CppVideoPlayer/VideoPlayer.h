#pragma once

#define __STDC_CONSTANT_MACROS

#include <stdlib.h>
#include <string>

extern "C"
{
	#include "libavutil/avutil.h"
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libswscale/swscale.h"
}

class VideoPlayer
{
	private:
		AVFormatContext* fFormatCtx;
		AVInputFormat* fInputFormat;
		AVCodecContext* fCodecCtx;
		AVCodec* fCurrentCodec;
		SwsContext* fSwsCtx;
		AVPacket* fPacket;
		AVFrame* fFrame;

	public:
		VideoPlayer();
		~VideoPlayer();
		int LoadFile(std::string fileName);
};

