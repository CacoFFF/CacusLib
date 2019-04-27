/*=============================================================================
	StackUnwinder.cpp
	Author: Fernando Velázquez

	Allows guarded code to throw exceptions in case of segmentation fault
	without having to use SEH.
=============================================================================*/


#include "Atomics.h"
#include "DebugCallback.h"
#include "StackUnwinder.h"
#include "CacusTemplate.h"

#include <signal.h>

#if _WINDOWS
	extern "C" __declspec(dllimport) uint32 __stdcall GetCurrentThreadId();
	#define GET_THREAD_ID() GetCurrentThreadId()
#else
	#include <unistd.h>
	#include <sys/syscall.h>
	#define GET_THREAD_ID() (uint32)syscall(__NR_gettid)
#endif

//******* Unwinder controller singleton
//
//This is required in order to prevent usage of thread_local which causes problems in Windows XP
//
struct UnwinderController
{
	struct UnwinderEntry
	{
		CUnwinder* List;
		uint32 ThreadId;

		UnwinderEntry() {}
		UnwinderEntry( CUnwinder* InUnwinder, uint32 InThreadId)
			:	List(InUnwinder), ThreadId(InThreadId) {}
	};
	volatile int32 Lock;
	size_t EntryCount;
	UnwinderEntry Entry[1024];  //TODO: Custom made hashmap

	UnwinderController()       : Lock(0), EntryCount(0) {}
	~UnwinderController()      { Lock = 0; }

	
	int Attach( CUnwinder* Unwinder); // Returns 1 if first
	void Detach( CUnwinder* Unwinder);
	void LongJump();
};
static UnwinderController Controller;



#if _WINDOWS
	#define SIGIOT SIGABRT
	#define SignalHandler(signal) SignalHandlerWindows(signal)
#else
	#define SignalHandler(signal) SignalHandlerLinux(signal ,siginfo_t* si, void* unused)
#endif

//******* Signal handlers.
//
// SIGSEGV: Restore environment and immediately throw an exception at beginning of guarded code.
// SIGIOT/SIGABRT: Exit app cleanly.
// In both cases callback is notified prior to action.
//
static void SignalHandler( int signal)
{
	if ( signal == SIGSEGV )
		DebugCallback( "Segmentation fault (SIGSEGV)", CACUS_CALLBACK_UNWINDER);
	else if ( signal == SIGIOT )
	{
		static uint32 Aborted = 0;
		if ( !Aborted++ )
		{
			DebugCallback( "Abort process (IOT)", CACUS_CALLBACK_UNWINDER);
			//Exit app cleanly
		}
		exit(0);
		return;
	}
	Controller.LongJump();
}


//******* This allows a thread to receive signals and handle them in custom ways.
//
// SIGSEGV: Segmentation fault has occured
// SIGIOT/SIGABRT: Program is requested termination.
//
void InstallSignalHandlers()
{
#if _WINDOWS
	signal( SIGSEGV, &SignalHandlerWindows);
	signal( SIGIOT, &SignalHandlerWindows);
#else
	static volatile int32 Installed = 0;
	if ( Installed++ ) //Will this work in windows
		return;

	//signal handler needs to be installed on every thread (???)
	struct sigaction action;
	action.sa_sigaction = &SignalHandlerLinux;
	action.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGIOT, &action, NULL);
#endif
}


//******* New stack unwinding object has been created
//
// See to which thread to this belong, then evaluate the need
// of allocating a new chain of unwinders or not.
//
CUnwinder::CUnwinder()
	: Prev(nullptr)
	, EnvId(0)
{
	if ( Controller.Attach(this) )
		InstallSignalHandlers();
}


//******* Stack unwinding object has succesfully expired
//
// Remove from chain, or delete chain if root unwinder
//
CUnwinder::~CUnwinder()
{
	Controller.Detach(this);
}


//***************************************************************
//******* UnwinderController implementation
//***************************************************************


//******* Attaches an unwinder to an unwinder chain
//
// Creates a new chain if this is the first unwinder in the thread
//
FORCEINLINE int UnwinderController::Attach( CUnwinder* Unwinder)
{
	uint32 ThreadId = GET_THREAD_ID();
	CSpinLock SL(&Lock);
	for ( size_t i=0 ; i<EntryCount ; i++ )
		if ( Entry[i].ThreadId == ThreadId )
		{
			Unwinder->EnvId = (uint32)i;
			Unwinder->Prev = Entry[i].List;
			Entry[i].List = Unwinder;
			return 0;
		}
	//This is a new thread
	if ( EntryCount == ARRAY_COUNT(Entry) )
		DebugCallback( "Created more than 1024 stack unwinder units!", CACUS_CALLBACK_UNWINDER|CACUS_CALLBACK_EXCEPTION);
	Unwinder->EnvId = (uint32)EntryCount;
	Entry[EntryCount++] = UnwinderEntry(Unwinder,ThreadId);
	return 1;
}
//******* Detaches an unwinder from an unwinder chain
//
// Removes the chain if it's the first unwinder
// NOTE: The signal handlers aren't uninstalled in this case!
//
FORCEINLINE void UnwinderController::Detach( CUnwinder* Unwinder)
{
	CSpinLock SL(&Lock);
	size_t i = Unwinder->EnvId;
	if ( i >= EntryCount )
		DebugCallback( "Stack unwinder Detach error (Bad EnvId), check for stack corruption.", CACUS_CALLBACK_UNWINDER|CACUS_CALLBACK_EXCEPTION);
	else if ( Entry[i].List != Unwinder )
		DebugCallback( "Stack unwinder Detach error (Inconsistance in chain), check that out-of-scope unwinder destruction is occuring (and only once per unwinder).", CACUS_CALLBACK_UNWINDER|CACUS_CALLBACK_EXCEPTION);
	else
	{
		if ( !Unwinder->Prev )
		{
			if ( i != (uint32)(EntryCount-1) ) //Not the last entry
			{
				Entry[i] = Entry[EntryCount-1]; //Correct
				for ( CUnwinder* Link=Entry[i].List ; Link ; Link=Link->Prev )
					Link->EnvId = (uint32)i;
			}
			EntryCount--;
		}
		else
			Entry[i].List = Unwinder->Prev;
	}
}

//******* Restores environment so that an exception is thrown in guarded code
//
// NOTE: Not all C++ objects are guaranteed to be destroyed here.
// You want to primarily use this to kill off a thread/app.
//
FORCEINLINE void UnwinderController::LongJump()
{
	uint32 ThreadId = GET_THREAD_ID();
	CUnwinder* CurrentUnwinder = nullptr;
	{
		CSpinLock SL(&Lock);
		for ( size_t i=0 ; i<EntryCount && !CurrentUnwinder ; i++ )
			if ( Entry[i].ThreadId == ThreadId )
				CurrentUnwinder = Entry[i].List;
	}
	if ( !CurrentUnwinder )
	{
		DebugCallback( "Stack unwinder Long Jump error (No Entry), SegFault occured outside of first guarded block!.", CACUS_CALLBACK_UNWINDER|CACUS_CALLBACK_EXCEPTION);
		return;
	}
	longjmp( CurrentUnwinder->Environment, 1);
}