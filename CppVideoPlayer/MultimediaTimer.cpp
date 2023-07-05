# include "MultimediaTimer.h"
#include <stdlib.h>
#include <exception>
#include <map>

auto gMultimediaTimerMap = new std::map<UINT, MultimediaTimer*>();

void CALLBACK HandleTimer(UINT wTimerID, UINT msg,
	DWORD dwUser, DWORD dw1, DWORD dw2)
{
	try
	{
		auto timer = gMultimediaTimerMap->find(wTimerID)->second;
		if (timer == nullptr)
			return;
		MultimediaTimerCallback callback = timer->GetCallback();
		if (callback != nullptr)
			callback(timer->GetOpaque());
	}
	catch (const std::exception&)
	{
		// Nothing to be done;
		return;
	}
}

MultimediaTimer::MultimediaTimer()
{
	fInterval = 5;
	fOpaque = nullptr;
	fCallback = nullptr;
}

MultimediaTimer::~MultimediaTimer()
{
	timeKillEvent(fTimerID);
}

void MultimediaTimer::SetResolution()
{
	LPTIMECALLBACK;
	TIMECAPS tc;
	UINT timerResolution;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
	{
		//
	}
	timerResolution = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
	timeBeginPeriod(timerResolution);
}

void MultimediaTimer::Start()
{
	fTimerID = timeSetEvent(fInterval, 1,
		(LPTIMECALLBACK)HandleTimer, (DWORD)0, TIME_PERIODIC);
	gMultimediaTimerMap->emplace(fTimerID, this);
}

void MultimediaTimer::Stop()
{
	if (fTimerID != 0)
	{
		timeKillEvent(fTimerID);
		fTimerID = 0;
	}
}

void MultimediaTimer::SetInterval(UINT interval)
{
	fInterval = interval;
}

UINT MultimediaTimer::GetInterval()
{
	return fInterval;
}

void MultimediaTimer::SetOpaque(void* opaque)
{
	fOpaque = opaque;
}

void* MultimediaTimer::GetOpaque()
{
	return fOpaque;
}

MultimediaTimerCallback MultimediaTimer::GetCallback()
{
	return fCallback;
}

void MultimediaTimer::SetCallback(MultimediaTimerCallback callback)
{
	fCallback = callback;
}