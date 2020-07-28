/*=============================================================================
	Atomics.h:
	Simple 32-bit atomic methods for multithreading.
	Based on Unreal Engine 4 atomics.
=============================================================================*/

#ifndef USES_CACUS_ATOMICS
#define USES_CACUS_ATOMICS

#include "CacusPlatform.h"

#ifdef __GNUC__

FORCEINLINE void Sleep( uint32 MilliSeconds )
{
	usleep( MilliSeconds * 1000 );
}

struct FLinuxPlatformAtomics
{
	static FORCEINLINE int32 InterlockedIncrement( volatile int32* Value )
	{
		return __sync_fetch_and_add(Value, 1) + 1;
	}
	static FORCEINLINE int32 InterlockedDecrement( volatile int32* Value )
	{
		return __sync_fetch_and_sub(Value, 1) - 1;
	}
	static FORCEINLINE int32 InterlockedAdd( volatile int32* Value, int32 Amount )
	{
		return __sync_fetch_and_add(Value, Amount);
	}
	static FORCEINLINE int32 InterlockedExchange( volatile int32* Value, int32 Exchange )
	{
		return __sync_lock_test_and_set(Value,Exchange);	
	}
	static FORCEINLINE int32 InterlockedCompareExchange( volatile int32* Dest, int32 Exchange, int32 Comparand )
	{
		return __sync_val_compare_and_swap(Dest, Comparand, Exchange);
	}
};
typedef FLinuxPlatformAtomics FPlatformAtomics;
#endif


#ifdef _MSC_VER

#include <intrin.h>
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedAdd
#undef InterlockedExchange
#undef InterlockedExchangeAdd
#undef InterlockedCompareExchange

#pragma warning (push)
#pragma warning (disable : 4035)

extern "C"
{
__declspec(dllimport) void __stdcall Sleep( unsigned long dwMilliseconds);
};


struct FWindowsPlatformAtomics
{
	static FORCEINLINE int InterlockedIncrement( volatile int32* Value )
	{
		return (int32)_InterlockedIncrement((long*)Value);
	}
	static FORCEINLINE int32 InterlockedDecrement( volatile int32* Value )
	{
		return (int32)_InterlockedDecrement((long*)Value);
	}
	static FORCEINLINE int32 InterlockedAdd( volatile int32* Value, int32 Amount )
	{
		return (int32)_InterlockedExchangeAdd((long*)Value, (long)Amount);
	}
	static FORCEINLINE int32 InterlockedExchange( volatile int32* Value, int32 Exchange )
	{
		return (int32)_InterlockedExchange((long*)Value, (long)Exchange);
	}
	static FORCEINLINE int32 InterlockedCompareExchange( volatile int32* Dest, int32 Exchange, int32 Comparand )
	{
		return (int32)_InterlockedCompareExchange((long*)Dest, (long)Exchange, (long)Comparand);
	}
};
typedef FWindowsPlatformAtomics FPlatformAtomics;

#pragma warning (pop)

#endif

class CSpinLock
{
	volatile int32 *Lock;

	CSpinLock() {} //No default constructor

public:	
	FORCEINLINE CSpinLock( volatile int32* InLock)
	:	Lock(InLock)
	{
		do
		{
			while ( *Lock); //Additional TEST operation before WRITE prevents scalability issues
		} while ( FPlatformAtomics::InterlockedCompareExchange(Lock, 1, 0) );
	}
	
	FORCEINLINE ~CSpinLock()
	{
		FPlatformAtomics::InterlockedExchange( Lock, 0);
	}
};

class CSleepLock
{
	volatile int32 *Lock;
	uint32 SleepInterval;

	CSleepLock() {} //No default constructor

public:	
	FORCEINLINE CSleepLock( volatile int32* InLock, uint32 InSleep=0)
	:	Lock(InLock)
	,	SleepInterval(InSleep)
	{
		do
		{
			while ( *Lock)
				Sleep(SleepInterval); //Additional TEST operation before WRITE prevents scalability issues
		} while ( FPlatformAtomics::InterlockedCompareExchange(Lock, 1, 0) );
	}
	
	FORCEINLINE ~CSleepLock()
	{
		FPlatformAtomics::InterlockedExchange( Lock, 0);
	}
};

// Increments/decrements within a scope with exception support
class CScopeCounter
{
	volatile int32 *Counter;

	CScopeCounter() {} //No default constructor

public:
	CScopeCounter( volatile int32* InCounter)
		: Counter(InCounter)
	{
		FPlatformAtomics::InterlockedIncrement(Counter);
	}

	~CScopeCounter()
	{
		FPlatformAtomics::InterlockedDecrement(Counter);
	}
};

// Atomic counter with limited manipulation
class CAtomicCounter
{
	volatile int32 Counter;
public:

	CAtomicCounter() : Counter(0) {}

	bool IncrementIfLesser( int32 Num)
	{
		if ( FPlatformAtomics::InterlockedIncrement( &Counter) <= Num )
			return true;
		FPlatformAtomics::InterlockedDecrement( &Counter);
		return false;
	}

	void Decrement()
	{
		FPlatformAtomics::InterlockedDecrement( &Counter);
	}
};


#endif