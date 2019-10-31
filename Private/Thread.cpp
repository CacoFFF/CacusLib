
#if _WINDOWS
	#include <Windows.h>
/*	extern "C"
	{
		__declspec(dllimport) void* __stdcall CreateThread
		(
			struct _SECURITY_ATTRIBUTES* lpThreadAttributes,
			uint32 dwStackSize,
			ENTRY_POINT lpStartAddress,
			void* lpParameter,
			uint32 dwCreationFlags,
			uint32* lpThreadId
		);

		__declspec(dllimport) int __stdcall CloseHandle
		(
			void* hObject
		);
	}*/


#else
#include <pthread.h>


#endif

#include "AppTime.h"
#include "Atomics.h"
#include "CacusThread.h"
#include "CacusGlobals.h"
#include "StackUnwinder.h"
#include "DebugCallback.h"


//---------- CThread::CThreadEntryContainer begin ----------//
ENTRY_TYPE CThread::CThreadEntryContainer( void* Arg)
{
	CScopeCounter SC( &COpenThreads);
	CThread* volatile Thread = (CThread*)Arg;

//	printf( "Run thread %i\n", Thread->tId);
	ENTRY_POINT EntryPoint     = Thread->EntryPoint;
	void*       EntryArg       = Thread->EntryArg;
	uint32      RepeatInterval = Thread->RepeatInterval;
	uint32      Flags          = Thread->Flags;
	uint32      tId            = Thread->ThreadId();
	Thread->ThreadHandlerPtr = &Thread;
	if ( Flags & THF_Detached ) //Detached
		Thread->Detach();

	uint32 Result = THREAD_END_NOT_OK;
	do
	{
		try
		{
			while ( EntryPoint )
			{
				Result = (EntryPoint)(EntryArg,Thread);

				if ( Result == THREAD_END_LOOP )
					{}
				else if ( Result == THREAD_END_REPEAT )
					Sleep( RepeatInterval);
				else
					break;
			}
		}
		catch (...)
		{
			char Buf[128];
			sprintf( Buf, "Exception caught on thread %i.", (int)tId);
			DebugCallback( Buf, CACUS_CALLBACK_THREAD);
			Result = THREAD_END_EXCEPTION;
		}
	}
	while ( (Flags & THF_ReentryOnExcept) && (Result == THREAD_END_EXCEPTION) );

	//If this is true, the thread handler no longer exists
	if ( Thread )
	{
//		printf("Thread ended with handler\n");
		Thread->Detach();
	}

	return 0;
}
//---------- CThread::CThreadEntryContainer end ----------//



CThread::CThread( const ENTRY_POINT InEntryPoint, void* InEntryArg, uint32 InFlags)
{
	memset( this, 0, (size_t)&EntryArg - (size_t)this);
	EntryArg = InEntryArg;
	EntryPoint = InEntryPoint;
	Flags = InFlags;
	if ( EntryPoint )
		Run();
}

//---------- CThread::~CThread begin ----------//
CThread::~CThread()
{
	WaitFinish();
	CSleepLock SL(&DestructLock);
//	printf("Thread destruct finish\n");
}
//---------- CThread::~CThread end ----------//


//---------- CThread::Run begin ----------//
int CThread::Run()
{
	if ( tId )
		return 0;
	CSpinLock SL(&Lock);
#if _WINDOWS
	void* Handle = CreateThread( nullptr, 0, CThread::CThreadEntryContainer, this, 0, (uint32*)&tId );
	if ( !Handle )
		return tId = 0;
	CloseHandle( Handle);
#else
	pthread_t Handle;
	pthread_attr_t ThreadAttributes;
	pthread_attr_init( &ThreadAttributes );
	pthread_attr_setdetachstate( &ThreadAttributes, PTHREAD_CREATE_DETACHED );
	if ( pthread_create( &Handle, &ThreadAttributes, CThread::CThreadEntryContainer, this ) )
		return tId = 0;
#endif
	FPlatformAtomics::InterlockedIncrement( &DestructLock);
	return 1;
}
//---------- CThread::Run end ----------//

//---------- CThread::Run begin ----------//
//
// <ENTRY_POINT> ThreadEntry - Changes thread entry
// <void*>       Arg         - Changes argument
//
int CThread::Run( ENTRY_POINT ThreadEntry, void* Arg)
{
	//Thread already running
	if ( tId )
		return 0;
	EntryPoint = ThreadEntry;
	EntryArg = Arg;
	return Run();
}
//---------- CThread::Run end ----------//

void CThread::Detach()
{
	CSpinLock SL(&Lock);
	if ( tId )
	{
//		printf( "Detach thread %i\n", tId);
		if ( ThreadHandlerPtr )
		{
			*ThreadHandlerPtr = nullptr;
			ThreadHandlerPtr = nullptr;
			FPlatformAtomics::InterlockedDecrement( &DestructLock);
		}
		tId = 0;
	}
}

int CThread::WaitFinish( float MaxWait)
{
	if ( tId == 0 )
		return 1;
	//Don't want joinable mumbo-jumbo
	//Sleep 1 ms at a time instead
	double StartTime = FPlatformTime::Seconds();
	while ( tId != 0 )
	{
		Sleep(1);
		if ( (MaxWait != 0) && (FPlatformTime::Seconds()-StartTime >= MaxWait) )
			break;
	}
	return tId == 0;
}