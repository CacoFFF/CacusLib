/*=============================================================================
	AppTime.h

	A simplified port of Unreal Engine 4's time platform.
=============================================================================*/

#ifndef USES_UE_TIME
#define USES_UE_TIME

#include "CacusPlatform.h"
#include "TCharBuffer.h"

#if _WINDOWS

#ifndef _INC_WINDOWS
typedef union _LARGE_INTEGER
{
	struct
	{
		uint32 LowPart;
		uint32 HighPart;
	} u;
	uint64 QuadPart;
} LARGE_INTEGER;

extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter( LARGE_INTEGER * lpPerformanceCount);
#endif

#elif __GNUC__

#include <sys/time.h>

#endif


//Unreal Engine 4 modification
struct CACUS_API FPlatformTime
{
	static double InitTiming();

#if _WINDOWS
	static inline double Seconds()
	{
		LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);

		// add big number to make bugs apparent where return value is being passed to float
		return Cycles.QuadPart * SecondsPerCycle + 16777216.0;
	}

	static inline uint32 Cycles()
	{
		LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);
		return Cycles.u.LowPart;
	}

	static inline uint64 Cycles64()
	{
		LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);
		return Cycles.QuadPart;
	}
#elif __GNUC__
	static inline double Seconds()
	{
		struct timeval tv;
		gettimeofday( &tv, NULL );
		return ((double) tv.tv_sec) + (((double) tv.tv_usec) / 1000000.0);
	}

	static inline uint32 Cycles()
	{
		struct timeval tv;
		gettimeofday( &tv, NULL );
		return (uint32) ((((uint64)tv.tv_sec) * 1000000ULL) + (((uint64)tv.tv_usec)));
	}

	static inline int64 Cycles64()
	{
		struct timeval tv;
		gettimeofday( &tv, NULL );
		return (uint64) ((((uint64)tv.tv_sec) * 1000000ULL) + (((uint64)tv.tv_usec)));
	}
#endif

	static void SystemTime( int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec );
	static void UtcTime( int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec );

	/**
	* Returns a timestamp string built from the current date and time.
	* NOTE: Only one return value is valid at a time!
	*
	* @return timestamp string
	**/
	static const char* StrTimestamp();

	// Returns a pretty-string for a time given in seconds. (I.e. "4:31 min", "2:16:30 hours", etc)
	static TCharBuffer<32,char> PrettyTime( double Seconds );

	static float ToMilliseconds( const uint32 Cycles )
	{
		return (float)(double( SecondsPerCycle * 1000 * Cycles));
	}

	static float ToSeconds( const uint32 Cycles )
	{
		return (float)(double( SecondsPerCycle * Cycles));
	}

	static float ToSeconds( const uint64 Cycles )
	{
		return (float)(double( SecondsPerCycle * Cycles));
	}

	static double GetSecondsPerCycle()
	{
		return SecondsPerCycle;
	}

protected:
	static uint64 StartCycles LINUX_SYMBOL(CacusTime_StartCycles);
	static double SecondsPerCycle LINUX_SYMBOL(CacusTime_SecondsPerCycle);
};


#endif