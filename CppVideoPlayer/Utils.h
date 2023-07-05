#pragma once

#include <string>
#include <sstream>

struct VideoDate
{
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
};

VideoDate FromSeconds(long long seconds)
{
	VideoDate vd;
	vd.hour = 0;
	vd.minute = 0;
	vd.second = 0;
	int i = 1;
	while (1)
	{
		if ((seconds / (i * 3600)) > 0)
			i++;
		else
			break;
	}
	vd.hour = i - 1;
	seconds -= (3600 * (i - 1));
	i = 1;
	while (1)
	{
		if ((seconds / (i * 60)) > 0)
			i++;
		else
			break;
	}
	vd.minute = i - 1;
	seconds -= (60 * (i - 1));
	vd.second = seconds;
	return vd;
}

std::wstring FormatVideoDate(VideoDate date)
{
	std::stringstream ss;
	if (date.hour < 10)
		ss << "0" << date.hour;
	else
		ss << date.hour;
	ss << ":";
	if (date.minute < 10)
		ss << "0" << date.minute;
	else
		ss << date.minute;
	ss << ":";
	if (date.second < 10)
		ss << "0" << date.second;
	else
		ss << date.second;
	auto s = ss.str();
	return std::wstring(s.begin(), s.end());
}
