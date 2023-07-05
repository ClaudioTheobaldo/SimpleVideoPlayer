#pragma once

#define __STDC_CONSTANT_MACROS

#include <stdlib.h>
#include <string>
#include "MultimediaTimer.h"

extern "C"
{
	#include "libavutil/avutil.h"
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libswscale/swscale.h"
}

typedef void (*ImgCallback) (char* imgBuffer, int width, int height, long long position);

struct FrameData
{
	char* data;
	int width;
	int height;
	long long position;
};

class VideoPlayer
{
	private:
		// Demuxer
		AVFormatContext* fFormatCtx;
		AVInputFormat* fInputFormat;
		// Decoder
		AVCodecContext* fCodecCtx;
		AVCodec* fCurrentCodec;
		// Scaler
		SwsContext* fSwsCtx;
		// Auxiliary variables
		AVPacket* fPacket;
		AVFrame* fDecodedFrame;
		AVFrame* fScaledFrame;

		int fOutputWidth;
		int fOutputHeight;
		
		int fVideoStreamIndex;

		bool fLoaded;
		bool fPlaying;
		bool fMightHaveMoreFrames;
		bool fAutoReset;
		bool fFinished;

		ImgCallback fCallback;
	public:
		VideoPlayer();
		~VideoPlayer();

		int LoadFile(const char* filename);
		bool Play();
		bool Stop();
		void Flush();

		FrameData* fFrameDataToShow;
		MultimediaTimer* fPlayTimer;
		long long delay;
		int64_t lastPTS;

		AVFrame* ReadNextFrame();
		FrameData* AVFrameToFrameData(AVFrame* frame);
		void ClearFrameDataToShow();

		void SetOutputWidth(int width);
		int GetOutputWidth();
		void SetOutputHeight(int height);
		int GetOutputHeight();
		float GetFrameRate();
		void SetImageCallback(ImgCallback callback);
		ImgCallback GetImageCallback();
		bool IsPlaying();
		long long GetStreamDuration();
		double GetStreamTimeBase();
		void Seek(uint64_t position);
		void SetAutoReset(bool shouldResetAfterEnd);
		bool GetAutoReset();
};

