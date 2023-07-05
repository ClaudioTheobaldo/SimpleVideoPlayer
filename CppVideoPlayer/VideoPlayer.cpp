#include "VideoPlayer.h"
#include <Windows.h>

extern "C"
{
	#include "libavutil/avutil.h"
	#include "libavutil/imgutils.h"
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libswscale/swscale.h"
}

void OnPlayerTimer(void* opaque)
{
	auto videoPlayer = static_cast<VideoPlayer*>(opaque);

	if (videoPlayer == nullptr)
		return;
	if (videoPlayer->GetImageCallback() == nullptr)
		return;

	if (videoPlayer->delay <= 0  && videoPlayer->lastPTS == 0)
	{
		auto frameOne = videoPlayer->ReadNextFrame();
		if (frameOne == nullptr)
			return;
		videoPlayer->fFrameDataToShow = videoPlayer->AVFrameToFrameData(frameOne);
		auto frameTwo = videoPlayer->ReadNextFrame();
		if (frameTwo == nullptr)
			return;

		auto timeBase = videoPlayer->GetStreamTimeBase() * 1000;
		videoPlayer->delay = (frameTwo->pts - videoPlayer->fFrameDataToShow->position) * timeBase;
		videoPlayer->GetImageCallback()(videoPlayer->fFrameDataToShow->data,
			videoPlayer->fFrameDataToShow->width, 
			videoPlayer->fFrameDataToShow->height, 
			videoPlayer->fFrameDataToShow->position);

		videoPlayer->ClearFrameDataToShow();

		videoPlayer->fFrameDataToShow = videoPlayer->AVFrameToFrameData(frameTwo);
		videoPlayer->lastPTS = videoPlayer->fFrameDataToShow->position;
	}
	else if (videoPlayer->delay <= 0 && videoPlayer->fFrameDataToShow != nullptr)
	{
		auto frame = videoPlayer->ReadNextFrame();
		if (frame == nullptr)
			return;
		auto frameToShow = videoPlayer->fFrameDataToShow;
		videoPlayer->fFrameDataToShow = videoPlayer->AVFrameToFrameData(frame);

		videoPlayer->delay = (frame->pts - videoPlayer->lastPTS) * (videoPlayer->GetStreamTimeBase() * 1000);
		videoPlayer->GetImageCallback()(frameToShow->data,
			frameToShow->width, frameToShow->height, frameToShow->position);

		free(frameToShow->data);
		delete frameToShow;

		videoPlayer->lastPTS = videoPlayer->fFrameDataToShow->position;
	}
	else
		videoPlayer->delay -= videoPlayer->fPlayTimer->GetInterval();
}

VideoPlayer::VideoPlayer()
{
	fOutputWidth = 320;
	fOutputHeight = 240;

	fPlayTimer = new MultimediaTimer();
	fPlayTimer->SetInterval(1);
	fPlayTimer->SetCallback(OnPlayerTimer);
	fPlayTimer->SetOpaque(static_cast<void*>(this));

	fLoaded = false;
	delay = 0;
	lastPTS = 0;
	fFrameDataToShow = nullptr;
	fMightHaveMoreFrames = false;
	fAutoReset = false;
}

VideoPlayer::~VideoPlayer()
{
	Flush();
	delete fPlayTimer;
}

int VideoPlayer::LoadFile(const char* filename)
{
	if (fLoaded)
		return -99;

	fFormatCtx = avformat_alloc_context();
	if (fFormatCtx == nullptr)
	{
		Flush();
		return -1;
	}

	if (avformat_open_input(&fFormatCtx, filename, nullptr, nullptr) != 0)
	{
		Flush();
		return -2;
	}

	fVideoStreamIndex = -1;
	fVideoStreamIndex = av_find_best_stream(fFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (fVideoStreamIndex == AVERROR_STREAM_NOT_FOUND)
	{
		Flush();
		return -3;
	}

	this->fCurrentCodec = (AVCodec*)avcodec_find_decoder(fFormatCtx->streams[fVideoStreamIndex]->codecpar->codec_id);
	if (this->fCurrentCodec == nullptr)
	{
		Flush();
		return -4;
	}

	fCodecCtx = avcodec_alloc_context3(this->fCurrentCodec);
	if (fCodecCtx == nullptr)
	{
		Flush();
		return -5;
	}

	if (avcodec_parameters_to_context(fCodecCtx, fFormatCtx->streams[fVideoStreamIndex]->codecpar) != 0)
	{
		Flush();
		return -6;
	}

	if (avcodec_open2(fCodecCtx, this->fCurrentCodec, nullptr) != 0)
	{
		Flush();
		return -7;
	}

	fPacket = av_packet_alloc();
	if (fPacket == nullptr)
	{
		Flush();
		return -8;
	}

	fDecodedFrame = av_frame_alloc();
	if (fDecodedFrame == nullptr)
	{
		Flush();
		return -9;
	}

	fScaledFrame = av_frame_alloc();
	if (fScaledFrame == nullptr)
	{
		Flush();
		return -10;
	}

	fScaledFrame->width = fOutputWidth;
	fScaledFrame->height = fOutputHeight;
	fScaledFrame->format = AV_PIX_FMT_BGR24;

	if (av_frame_get_buffer(fScaledFrame, 0) != 0)
	{
		Flush();
		return -11;
	}

	fLoaded = true;
	return 0;
}

AVFrame* VideoPlayer::ReadNextFrame()
{
	if (!fLoaded)
		return nullptr;

	if (fMightHaveMoreFrames)
	{
		auto r = avcodec_receive_frame(fCodecCtx, fDecodedFrame);
		if (r == 0)
		{
			if (fSwsCtx == nullptr)
			{
				fSwsCtx = sws_getContext(fDecodedFrame->width, fDecodedFrame->height,
					(AVPixelFormat)fDecodedFrame->format, fScaledFrame->width,
					fScaledFrame->height, (AVPixelFormat)fScaledFrame->format,
					SWS_BICUBIC, nullptr, nullptr, nullptr);
				if (fSwsCtx == nullptr)
					return nullptr;
			}
			r = sws_scale(fSwsCtx, fDecodedFrame->data, fDecodedFrame->linesize, 0,
				fDecodedFrame->height, fScaledFrame->data, fScaledFrame->linesize);
			av_frame_copy_props(fScaledFrame, fDecodedFrame);
			av_frame_unref(fDecodedFrame);
			return fScaledFrame;
		}
		else
			fMightHaveMoreFrames = false;
	}

	int rr = 0;
	while (true)
	{
		rr = av_read_frame(fFormatCtx, fPacket);

		if (rr != 0)
			break;

		if (fPacket->stream_index != fVideoStreamIndex)
			continue;
		auto r = avcodec_send_packet(fCodecCtx, fPacket);
		av_packet_unref(fPacket);
		if (r == 0)
		{
			r = avcodec_receive_frame(fCodecCtx, fDecodedFrame);
			if (r == 0)
			{
				if (fSwsCtx == nullptr)
				{
					fSwsCtx = sws_getContext(fDecodedFrame->width, fDecodedFrame->height,
						(AVPixelFormat)fDecodedFrame->format, fScaledFrame->width,
						fScaledFrame->height, (AVPixelFormat)fScaledFrame->format,
						SWS_BICUBIC, nullptr, nullptr, nullptr);
					if (fSwsCtx == nullptr)
						return nullptr;
				}
				r = sws_scale(fSwsCtx, fDecodedFrame->data, fDecodedFrame->linesize, 0,
					fDecodedFrame->height, fScaledFrame->data, fScaledFrame->linesize);
				av_frame_copy_props(fScaledFrame, fDecodedFrame);
				av_frame_unref(fDecodedFrame);
				fMightHaveMoreFrames = true;
				return fScaledFrame;
			}
		}
	}

	if (rr == AVERROR_EOF)
	{
		if (fAutoReset)
			Seek(0);
		else
		{
			Stop();
			fFinished = true;
		}
	}

	return nullptr;
}

bool VideoPlayer::Play()
{
	if (!fLoaded)
		return false;

	if (fFinished)
	{
		fFinished = false;
		Seek(0);
	}

	fPlayTimer->Start();
	fPlaying = true;
	return true;
}

bool VideoPlayer::Stop()
{
	if (!fPlaying)
		return false;
	fPlayTimer->Stop();
	fPlaying = false;
	return true;
}

void VideoPlayer::Flush()
{
	Stop();
	if (fFormatCtx != nullptr)
		avformat_close_input(&fFormatCtx);
	if (fCodecCtx != nullptr)
	{
		avcodec_close(fCodecCtx);
		avcodec_free_context(&fCodecCtx);
	}
	this->fCurrentCodec = nullptr;
	if (fPacket != nullptr)
		av_packet_free(&fPacket);
	if (fDecodedFrame != nullptr)
		av_frame_free(&fDecodedFrame);
	if (fScaledFrame != nullptr)
		av_frame_free(&fScaledFrame);
	if (fSwsCtx != nullptr)
	{
		sws_freeContext(fSwsCtx);
		fSwsCtx = nullptr;
	}
	fLoaded = false;
	fMightHaveMoreFrames = false;
	if (fFrameDataToShow != nullptr)
		ClearFrameDataToShow();
	lastPTS = 0;
	delay = 0;
	fFinished = 0;
}

void VideoPlayer::SetOutputWidth(int width)
{
	fOutputWidth = width;
}

int VideoPlayer::GetOutputWidth()
{
	return fOutputWidth;
}

void VideoPlayer::SetOutputHeight(int height)
{
	fOutputHeight = height;
}

int VideoPlayer::GetOutputHeight()
{
	return fOutputHeight;
}

float VideoPlayer::GetFrameRate()
{
	if (!fLoaded)
		return -1.00f;
	{
		AVStream* st = fFormatCtx->streams[fVideoStreamIndex];
		return (float)(st->avg_frame_rate.num / st->avg_frame_rate.den);
	}
}

void VideoPlayer::SetImageCallback(ImgCallback callback)
{
	fCallback = callback;
}

ImgCallback VideoPlayer::GetImageCallback()
{
	return fCallback;
}

bool VideoPlayer::IsPlaying()
{
	return fPlaying;
}

long long VideoPlayer::GetStreamDuration()
{
	if (!fLoaded)
		return 0;
	auto timeBase = fFormatCtx->streams[fVideoStreamIndex]->time_base;
	return fFormatCtx->streams[fVideoStreamIndex]->duration * av_q2d(timeBase);
}

double VideoPlayer::GetStreamTimeBase()
{
	if (!fLoaded)
		return 0;
	return av_q2d(fFormatCtx->streams[fVideoStreamIndex]->time_base);
}

void VideoPlayer::Seek(uint64_t position)
{
	if (!fLoaded)
		return;
	uint64_t scaledPos = fFormatCtx->streams[fVideoStreamIndex]->time_base.den * position;
	auto r = av_seek_frame(fFormatCtx, fVideoStreamIndex, scaledPos, 0);
	avcodec_flush_buffers(fCodecCtx);
	if (this->fFrameDataToShow != nullptr)
		ClearFrameDataToShow();
	delay = 0;
	lastPTS = 0;
}

FrameData* VideoPlayer::AVFrameToFrameData(AVFrame* frame)
{
	auto frameData = new FrameData();
	frameData->data = (char*)malloc(sizeof(char) * (frame->width * frame->height * 3));
	memcpy(frameData->data, frame->data[0], (frame->width * frame->height * 3));
	frameData->height = frame->height;
	frameData->width = frame->width;
	frameData->position = frame->pts;
	return frameData;
}

void VideoPlayer::ClearFrameDataToShow()
{
	free(this->fFrameDataToShow->data);
	delete this->fFrameDataToShow;
	this->fFrameDataToShow = nullptr;
}

void VideoPlayer::SetAutoReset(bool shouldResetAfterEnd)
{
	fAutoReset = shouldResetAfterEnd;
}

bool VideoPlayer::GetAutoReset()
{
	return fAutoReset;
}