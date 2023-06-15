#include "VideoPlayer.h"

extern "C"
{
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

VideoPlayer::VideoPlayer()
{

}

VideoPlayer::~VideoPlayer()
{

}

int VideoPlayer::LoadFile(std::string fileName)
{
	fFormatCtx = avformat_alloc_context();
	if (fFormatCtx == nullptr)
		return -1;
	auto r = avformat_open_input(&fFormatCtx, fileName.c_str(), fInputFormat, nullptr);
	if (r != 0)
		return -2;
	return 0;
}