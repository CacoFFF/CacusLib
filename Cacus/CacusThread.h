/*=============================================================================
	CacusThread.h:
	Thread abstractions.
=============================================================================*/

#ifndef INC_CTHREAD
#define INC_CTHREAD

#include "CacusPlatform.h"
#include "Atomics.h"

//Types are all 32 bits

#ifdef _WINDOWS
	#define ENTRY_TYPE uint32 __stdcall
#elif __GNUC__
	#include <sys/types.h>
	#define ENTRY_TYPE void*
#endif

#define THREAD_END_OK         ((uint32)  0UL)
#define THREAD_END_NOT_OK     ((uint32)  1UL)
#define THREAD_END_EXCEPTION  ((uint32)  2UL)
#define THREAD_END_REPEAT     ((uint32)100UL)
#define THREAD_END_LOOP       ((uint32)101UL)

//THREAD_END_REPEAT sleeps(0) before re-entrying
//THREAD_END_LOOP retries immediately

enum EThreadFlags
{
	THF_ReentryOnExcept    = 0x0001,
	THF_DeleteOnEnd        = 0x0002, //DESTRUCTOR IS NOT VIRTUAL
	THF_NoThrow            = 0x0004, //TODO
	THF_Detached           = 0x0008,
};

//////////////////////////////////////////
// Generic thread (Cacus)
// Non-templated, so entry point needs to be defined using THREAD_ENTRY
// A lambda expression can be used as well: [](void* Arg, CThread* Handler){ ... return THREAD_END_OK; }
//////////////////////////////////////////
struct CACUS_API CThread
{
	typedef uint32 (*ENTRY_POINT)(void*,CThread*);
private:
	CAtomicLock Lock;
	CAtomicLock DestructLock;
	volatile uint32 tId;
	CThread* volatile* volatile ThreadHandlerPtr; // *ThreadHandlerPtr == this
public:
	uint32 RepeatInterval;

	void* EntryArg;
	ENTRY_POINT EntryPoint;
	uint32 Flags;

	CThread( const ENTRY_POINT InEntryPoint=nullptr, void* InEntryArg=nullptr, uint32 InFlags=0 );
	~CThread();

	int Run();
	int Run( ENTRY_POINT ThreadEntry, void* Arg=nullptr);
	void Detach();
	int WaitFinish( float MaxWait=0);

	bool IsEnded() { return tId == 0; }
	uint32 ThreadId() { return tId; }
private:
	static ENTRY_TYPE CThreadEntryContainer( void* Arg);
};





#endif