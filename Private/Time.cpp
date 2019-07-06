#include "CacusField.h"
#include "CacusPlatform.h"
#include "AppTime.h"
#include "TimeStamp.h"
#include "StackUnwinder.h"

#include <string>

double FPlatformTime::SecondsPerCycle = 0.0;

#if _WINDOWS

typedef struct _SYSTEMTIME {
	uint16 wYear;
	uint16 wMonth;
	uint16 wDayOfWeek;
	uint16 wDay;
	uint16 wHour;
	uint16 wMinute;
	uint16 wSecond;
	uint16 wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

extern "C" __declspec(dllimport) void __stdcall GetLocalTime( LPSYSTEMTIME lpSystemTime);
extern "C" __declspec(dllimport) void __stdcall GetSystemTime( LPSYSTEMTIME lpSystemTime);
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency( LARGE_INTEGER * lpPerformanceCount);

double FPlatformTime::InitTiming()
{
	if ( FPlatformTime::SecondsPerCycle == 0.0 )
	{
		LARGE_INTEGER Frequency;
		QueryPerformanceFrequency(&Frequency);
		SecondsPerCycle = 1.0 / Frequency.QuadPart;
	}
	return FPlatformTime::Seconds();
}

void FPlatformTime::SystemTime( int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec )
{
	SYSTEMTIME st;
	GetLocalTime( &st );

	Year		= st.wYear;
	Month		= st.wMonth;
	DayOfWeek	= st.wDayOfWeek;
	Day			= st.wDay;
	Hour		= st.wHour;
	Min			= st.wMinute;
	Sec			= st.wSecond;
	MSec		= st.wMilliseconds;
}


void FPlatformTime::UtcTime( int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec )
{
	SYSTEMTIME st;
	GetSystemTime( &st );

	Year		= st.wYear;
	Month		= st.wMonth;
	DayOfWeek	= st.wDayOfWeek;
	Day			= st.wDay;
	Hour		= st.wHour;
	Min			= st.wMinute;
	Sec			= st.wSecond;
	MSec		= st.wMilliseconds;
}

#elif __GNUC__

double FPlatformTime::InitTiming()
{
	// we use gettimeofday() instead of rdtsc, so it's 1000000 "cycles" per second on this faked CPU.
	SecondsPerCycle = 1.0f / 1000000.0f;
	return FPlatformTime::Seconds();
}

void FPlatformTime::SystemTime( int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec )
{
	// query for calendar time
	struct timeval Time;
	gettimeofday(&Time, NULL);

	// convert it to local time
	struct tm LocalTime;
	localtime_r(&Time.tv_sec, &LocalTime);

	// pull out data/time
	Year		= LocalTime.tm_year + 1900;
	Month		= LocalTime.tm_mon + 1;
	DayOfWeek	= LocalTime.tm_wday;
	Day			= LocalTime.tm_mday;
	Hour		= LocalTime.tm_hour;
	Min			= LocalTime.tm_min;
	Sec			= LocalTime.tm_sec;
	MSec		= Time.tv_usec / 1000;
}

void FPlatformTime::UtcTime( int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec )
{
	// query for calendar time
	struct timeval Time;
	gettimeofday(&Time, NULL);

	// convert it to UTC
	struct tm LocalTime;
	gmtime_r(&Time.tv_sec, &LocalTime);

	// pull out data/time
	Year		= LocalTime.tm_year + 1900;
	Month		= LocalTime.tm_mon + 1;
	DayOfWeek	= LocalTime.tm_wday;
	Day			= LocalTime.tm_mday;
	Hour		= LocalTime.tm_hour;
	Min			= LocalTime.tm_min;
	Sec			= LocalTime.tm_sec;
	MSec		= Time.tv_usec / 1000;
}

#endif


const char* FPlatformTime::StrTimestamp()
{
	int32 Year;
	int32 Month;
	int32 DayOfWeek;
	int32 Day;
	int32 Hour;
	int32 Min;
	int32 Sec;
	int32 MSec;

	FPlatformTime::SystemTime(Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec );
	return CSprintf( "%02d/%02d/%02d %02d:%02d:%02d", Day, Month, Year%100, Hour, Min, Sec);
}

/**
* Returns a pretty-string for a time given in seconds. (I.e. "4:31 min", "2:16:30 hours", etc)
* @param Seconds	Time in seconds
* @return			Time in a pretty formatted string
*/
TChar8Buffer<32> FPlatformTime::PrettyTime( double Seconds )
{
	TChar8Buffer<32> Result;
/*	if ( Seconds < 1.0 )
	{
	return FString::Printf( TEXT("%d ms"), FMath::TruncToInt(Seconds*1000) );
	}
	else if ( Seconds < 10.0 )
	{
	int32 Sec = FMath::TruncToInt(Seconds);
	int32 Ms = FMath::TruncToInt(Seconds*1000) - Sec*1000;
	return FString::Printf( TEXT("%d.%02d sec"), Sec, Ms/10 );
	}
	else if ( Seconds < 60.0 )
	{
	int32 Sec = FMath::TruncToInt(Seconds);
	int32 Ms = FMath::TruncToInt(Seconds*1000) - Sec*1000;
	return FString::Printf( TEXT("%d.%d sec"), Sec, Ms/100 );
	}
	else if ( Seconds < 60.0*60.0 )
	{
	int32 Min = FMath::TruncToInt(Seconds/60.0);
	int32 Sec = FMath::TruncToInt(Seconds) - Min*60;
	return FString::Printf( TEXT("%d:%02d min"), Min, Sec );
	}
	else
	{
	int32 Hr = FMath::TruncToInt(Seconds/3600.0);
	int32 Min = FMath::TruncToInt((Seconds - Hr*3600)/60.0);
	int32 Sec = FMath::TruncToInt(Seconds - Hr*3600 - Min*60);
	return FString::Printf( TEXT("%d:%02d:%02d hours"), Hr, Min, Sec );
	}*/
	return Result;
}


//**********************************************
// FTimestampDateTime
//

static uint8 _days_month_[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

FTimestampDateTime::FTimestampDateTime()
{
	*(uint64*)this = 0ULL;
}

FTimestampDateTime::FTimestampDateTime(ENow)
{
	int32 year, month, dayofweek, day, hour, min, sec, msec;
	FPlatformTime::SystemTime( year, month, dayofweek, day, hour, min, sec, msec );
	Year = year;
	Month = month;
	Day = day;
	Hour = hour;
	Minute = min;
	Second = sec;
}

TChar8Buffer<32> FTimestampDateTime::operator*()
{
	//	sizeof "YYYY-MM-DDTHH:mm:ss.000Z"
	TChar8Buffer<32> Result;
	sprintf( *Result, "%04d-%02d-%02dT%02d:%02d:%02d.000Z", (int)Year, (int)Month, (int)Day, (int)Hour, (int)Minute, (int)Second);
	return Result;
}

int64 FTimestampDateTime::Seconds()
{
	uint64 Result =
		+ Second
		+ Minute * 60
		+ Hour * 60 * 60
		+ (uint64)Days() * 60 * 60 * 24;
	return (int64)Result;
}

uint32 FTimestampDateTime::Days()
{
	if ( Day == 0 )
		return 0;
	uint32 Result = (uint32)Day - 1; // Day 1 is first day, so not full day yet
	for ( uint32 i=1 ; i<Month ; i++ ) //Sum days of passed months
		Result += (uint32)_days_month_[i];
	//29th Feb if we're past that date
	const uint32 Years = Year;
	Result += (Month > 2) && LeapYear(Years);
	//Branchless leap days adder
	Result += Years / 4;
	Result -= Years / 100;
	Result += Years / 400;
	return Result;
}

FTimestampDateTime FTimestampDateTime::Parse( const char* Str)
{
	FTimestampDateTime Res;
	TChar8Buffer<32> Buffer(Str);
	uint32 Len = Buffer.Len();

	if ( Len >= 10 )  //"2018-07-15T04:00:00.000Z"
	{
		FTimestampDateTime New;

		bool bHasTime = (Buffer[10] == 'T');
		//Year is 4 digits
		if ( (Buffer[4] == '-') && (Buffer[7] == '-') && (bHasTime || !Buffer[10]) )
		{
			Buffer[4] = Buffer[7] = Buffer[10] = 0;
			New.Year = atoi(*Buffer);
			New.Month = atoi(*Buffer + 5);
			New.Day = atoi(*Buffer + 8);
			Res = New;
			if ( bHasTime && (Len >= 13) && (Buffer[13] == ':' || !Buffer[13]) )
			{
				Buffer[13] = 0;
				Res.Hour = atoi(*Buffer + 11);
				if ( (Len >= 16) && (Buffer[16] == ':' || !Buffer[16]) )
				{
					Buffer[16] = 0;
					Res.Minute = atoi(*Buffer + 14);
					if ( (Len >= 19) && (Buffer[19] == '.' || !Buffer[19]) )
					{
						Buffer[19] = 0;
						Res.Second = atoi(*Buffer + 17);
					}
				}
			}
		}
	}
	return Res;
}

bool FTimestampDateTime::LeapYear(uint32 Year)
{
	if ( Year % 4 == 0 ) //Year not divisible by 4
		return false;
	else if ( Year % 100 == 0 ) //Year not divisible by 100
		return true;
	else if ( Year % 400 == 0 ) //Year not divisible by 400
		return false;
	else
		return true;
}

uint32 FTimestampDateTime::DaysPerMonth( uint32 Month, uint32 Year)
{
	if ( Month < 1 || Month > 12 )
		return 0;
	if ( Month == 2 && LeapYear(Year) )
		return 29;
	return _days_month_[Month];
}
