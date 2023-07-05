#include <windows.h>
#include <timeapi.h>

#pragma once

class MultimediaTimer;

typedef void (*MultimediaTimerCallback)(void* opaque);

class MultimediaTimer
{
	private:
		UINT fInterval = 0;
		UINT fTimerID = 0;
		void* fOpaque;
		MultimediaTimerCallback fCallback;
	public:
		MultimediaTimer();
		~MultimediaTimer();
		void SetResolution();
		void Start();
		void Stop();
		void SetInterval(UINT interval);
		UINT GetInterval();
		void SetOpaque(void* opaque);
		void* GetOpaque();
		MultimediaTimerCallback GetCallback();
		void SetCallback(MultimediaTimerCallback callback);
};

