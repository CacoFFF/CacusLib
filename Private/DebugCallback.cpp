
#include "DebugCallback.h"
#include "Atomics.h"
#include <vector>

struct CallbackEntry
{
	CACUS_DEBUG_CALLBACK_FUNC Function;
	int Flags;

	CallbackEntry( CACUS_DEBUG_CALLBACK_FUNC InFunction, int InFlags) : Function(InFunction), Flags(InFlags) {}
};

static volatile int32 Lock = 0;
static std::vector<CallbackEntry> CallbackList;

void DebugCallback( const char* Msg, int MsgType)
{
	if ( CallbackList.size() )
	{
		CSleepLock SL( &Lock);
		for ( size_t i=0 ; i<CallbackList.size() ; i++ )
		{
			//Debug calls must only be printed in debug callbacks
			if ( (MsgType & CACUS_CALLBACK_DEBUGONLY) && (CallbackList[i].Flags & CACUS_CALLBACK_DEBUGONLY) ) 
			{
				if ( CallbackList[i].Flags & MsgType & CACUS_CALLBACK_ALL )
					(*CallbackList[i].Function)(Msg,MsgType);
			}
			//Non-debug calls must only be printed in non-debug callbacks
			else if ( !(MsgType & CACUS_CALLBACK_DEBUGONLY) && !(CallbackList[i].Flags & CACUS_CALLBACK_DEBUGONLY) )
			{
				if ( CallbackList[i].Flags & MsgType ) 
					(*CallbackList[i].Function)(Msg,MsgType);
			}
		}
	}
}


int CDbg_RegisterCallback( CACUS_DEBUG_CALLBACK_FUNC Callback, int CallbackFlags, int Slot)
{
	if ( CallbackList.size() >= MAXINT )
		return CDBG_ERROR;

	size_t i = static_cast<size_t>(Slot);
	size_t j = static_cast<size_t>(CDbg_SlotCallback( Callback));
	if ( j < CallbackList.size() ) //Found
	{
		if ( i == j )
		{
			CallbackList[i].Flags = CallbackFlags;
			return CDBG_OK;
		}
		CallbackList.erase( CallbackList.begin() + j);
	}
	CallbackEntry NewEntry( Callback, CallbackFlags);
	if ( i >= CallbackList.size() )
		CallbackList.push_back( NewEntry);
	else
		CallbackList.insert( CallbackList.begin() + i, NewEntry);
	return CDBG_OK;
}


int CDbg_UnregisterCallback( CACUS_DEBUG_CALLBACK_FUNC Callback)
{
	int Slot = CDbg_SlotCallback( Callback);
	if ( Slot >= 0 )
		CallbackList.erase( CallbackList.begin() + static_cast<size_t>(Slot));
	return CDBG_OK;
}


int CDbg_SlotCallback( CACUS_DEBUG_CALLBACK_FUNC Callback)
{
	CSleepLock SL( &Lock);
	for ( size_t i=0 ; i<CallbackList.size() ; i++ )
		if ( CallbackList[i].Function == Callback )
			return static_cast<int>(i);
	return INDEX_NONE;
}

void CDbg_UnlockCallback()
{
	FPlatformAtomics::InterlockedExchange( &Lock, 0);
}


