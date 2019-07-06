
#include "DebugCallback.h"
#include "Atomics.h"

struct CallbackEntry
{
	CACUS_DEBUG_CALLBACK_FUNC Function;
	int Flags;
	CallbackEntry* Next;

	CallbackEntry( CACUS_DEBUG_CALLBACK_FUNC InFunction, int InFlags) : Function(InFunction), Flags(InFlags) {}
};
static volatile int32 Lock = 0;
static CallbackEntry* CallbackList = nullptr;

void DebugCallback( const char* Msg, int MsgType)
{
	CSleepLock SL( &Lock);
	for ( CallbackEntry* Link=CallbackList ; Link ; Link=Link->Next )
	{
		CallbackEntry& Callback = *Link;
		//Debug calls must only be printed in debug callbacks
		if ( (MsgType & CACUS_CALLBACK_DEBUGONLY) && (Callback.Flags & CACUS_CALLBACK_DEBUGONLY) ) 
		{
			if ( Callback.Flags & MsgType & CACUS_CALLBACK_ALL )
				(*Callback.Function)(Msg,MsgType);
		}
		//Non-debug calls must only be printed in non-debug callbacks
		else if ( !(MsgType & CACUS_CALLBACK_DEBUGONLY) && !(Callback.Flags & CACUS_CALLBACK_DEBUGONLY) )
		{
			if ( Callback.Flags & MsgType ) 
				(*Callback.Function)(Msg,MsgType);
		}
	}
}


int CDbg_RegisterCallback( CACUS_DEBUG_CALLBACK_FUNC Callback, int CallbackFlags, int Slot)
{
	int i;
	CallbackEntry** ListPtr;
	CSleepLock SL( &Lock);
	CallbackEntry* NewEntry = nullptr;

	//Find existing entry
	for ( ListPtr=&CallbackList, i=0 ; *ListPtr ; ListPtr=&((*ListPtr)->Next), i++ )
	{
		CallbackEntry* Entry = *ListPtr;
		if ( Entry->Function == Callback )
		{
			if ( i == Slot ) //Same slot, modify flags
			{
				Entry->Flags = CallbackFlags;
				return CDBG_OK;
			}
			//Diff slot, unlink
			*ListPtr = Entry->Next;
			NewEntry = Entry;
			break;
		}
	}

	// Setup entry
	if ( !NewEntry )
	{
		NewEntry = (CallbackEntry*)CMalloc( sizeof(CallbackEntry) );
		NewEntry->Function = Callback;
	}
	NewEntry->Flags = CallbackFlags;
	NewEntry->Next = nullptr;

	// Find where to chain this
	for ( ListPtr=&CallbackList, i=0 ; *ListPtr ; ListPtr=&((*ListPtr)->Next), i++ )
		if ( i == Slot )
		{
			NewEntry->Next = *ListPtr;
			break;
		}
	*ListPtr = NewEntry;
	return CDBG_OK;
}


int CDbg_UnregisterCallback( CACUS_DEBUG_CALLBACK_FUNC Callback)
{
	CSleepLock SL( &Lock);
	for ( CallbackEntry** ListPtr=&CallbackList ; *ListPtr ; ListPtr=&((*ListPtr)->Next) )
	{
		CallbackEntry* Entry = *ListPtr;
		if ( Entry->Function == Callback )
		{
			*ListPtr = Entry->Next;
			CFree( Entry);
			return CDBG_OK;
		}
	}
	return CDBG_ERROR;
}


int CDbg_SlotCallback( CACUS_DEBUG_CALLBACK_FUNC Callback)
{
	CSleepLock SL( &Lock);
	int i = INDEX_NONE;
	for ( CallbackEntry* Link=CallbackList ; Link ; Link=Link->Next )
	{
		i++;
		if ( Link->Function == Callback )
			break;
	}
	return i;
}

void CDbg_UnlockCallback()
{
	FPlatformAtomics::InterlockedExchange( &Lock, 0);
}


