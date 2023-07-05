#pragma once

#include <windows.h>

typedef void (*TimerFunc)(void* opaque);

class WinTimer
{
	private:
		UINT_PTR fTimerID;
		UINT fInterval;
		TimerFunc fTimerCallback;
		void* fOpaque;
		bool fTicking;
	public:
		WinTimer();
		~WinTimer();
		void Start();
		void Stop();
		void SetInterval(UINT ms);
		UINT GetInterval();
		TimerFunc GetTimerCallback();
		void SetTimerCallback(TimerFunc fn);
		void* GetOpaque();
		void SetOpaque(void* opaque);
		bool IsTicking();
};
