

#include "Atomics.h"
#include "CacusTemplate.h"
#include "CacusOutputDevice.h"

#include "StackUnwinder.h"

#include <signal.h>


//******* Thread statics
//
// Points to the highest (most recent) element in the chain
//
thread_local CUnwinder* UnwinderChain = nullptr;


//******* Signal handlers
//
#if _WINDOWS
#define SIGIOT SIGABRT
static void SignalHandlerWindows( int signal)
#else
static void SignalHandlerLinux( int signal, siginfo_t *si, void *unused)
#endif
{
	if ( signal == SIGSEGV )
		Cdebug( "Segmentation fault (SIGSEGV)");
	else if ( signal == SIGIOT )
	{
		static uint32 Aborted = 0;
		if ( !Aborted++ )
		{
			Cdebug( "Abort process (IOT)");
			//Exit app cleanly
		}
		exit(0);
		return;
	}
	if ( UnwinderChain )
		longjmp( UnwinderChain->Environment, 1);
}


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
	: Last(nullptr)
	, Next(nullptr)
{
	if ( UnwinderChain )
	{
		Last = UnwinderChain;
		Last->Next = this;
	}
	else
		InstallSignalHandlers();
	UnwinderChain = this;
}


//******* Stack unwinding object has succesfully expired
//
// Remove from chain, or delete chain if root unwinder
//
CUnwinder::~CUnwinder()
{
	if ( Last )
	{
		Last->Next = nullptr;
		UnwinderChain = Last;
	}
	else
		UnwinderChain = nullptr;
}