#include "WinTimer.h"
#include <windows.h>
#include <WinUser.h>
#include <map>
#include <mmsystem.h>
#include <timeapi.h>

auto fTimerMap = new std::map<UINT_PTR, void*>();

void CALLBACK WinTimerOnTimer(HWND handle, UINT message,
	UINT_PTR idTimer, DWORD dwTime)
{
	auto pair = fTimerMap->find(idTimer);
	if (pair->second != nullptr)
	{
		WinTimer* timer = static_cast<WinTimer*>(pair->second);
		TimerFunc fn = timer->GetTimerCallback();
		fn(timer->GetOpaque());
	}
}

WinTimer::WinTimer()
{
	fTimerID = 0;
	fInterval = 1000;
	fTimerCallback = nullptr;
	fOpaque = nullptr;
	fTicking = false;
}

WinTimer::~WinTimer()
{
	Stop();
}

void WinTimer::Start()
{
	fTimerID = SetTimer(NULL, NULL, fInterval, (TIMERPROC)WinTimerOnTimer);
	fTimerMap->emplace(fTimerID, (void*)this);
	fTicking = true;
}

void WinTimer::Stop()
{
	fTimerMap->erase(fTimerID);
	KillTimer(NULL, fTimerID);
	fTicking = false;
}

void WinTimer::SetInterval(UINT ms)
{
	fInterval = ms;
}

UINT WinTimer::GetInterval()
{
	return fInterval;
}

TimerFunc WinTimer::GetTimerCallback()
{
	return fTimerCallback;
}

void WinTimer::SetTimerCallback(TimerFunc fn)
{
	fTimerCallback = fn;
}

void* WinTimer::GetOpaque()
{
	return fOpaque;
}

void WinTimer::SetOpaque(void* opaque)
{
	fOpaque = opaque;
}

bool WinTimer::IsTicking()
{
	return fTicking;
}