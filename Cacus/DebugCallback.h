/*=============================================================================
	DebugCallback.h
	Author: Fernando Velázquez

	If you want logs from the library register your logger here.
	The callbacks are always thread-safe.
	The registering operations are not thread-safe.
=============================================================================*/

#ifndef CACUS_DEBUG_CALLBACK
#define CACUS_DEBUG_CALLBACK

#include "CacusPlatform.h"

#define CACUS_CALLBACK_STRING     0x00000001
#define CACUS_CALLBACK_PARSER     0x00000002
#define CACUS_CALLBACK_THREAD     0x00000004
#define CACUS_CALLBACK_URI        0x00000008
#define CACUS_CALLBACK_UNWINDER   0x00000010
#define CACUS_CALLBACK_TEST       0x00000020 //Unit tests send these notifications
#define CACUS_CALLBACK_MEMORY     0x00000040
#define CACUS_CALLBACK_IO         0x00000080
#define CACUS_CALLBACK_EXCEPTION  0x00010000
#define CACUS_CALLBACK_ALL        0x0FFFFFFF

// Debug calls (more verbose stuff) are exclusive to debug handlers, handler MUST have
// the CACUS_CALLBACK_DEBUGONLY flag to receive specific debug callbacks of the above types
#define CACUS_CALLBACK_DEBUGONLY  0x10000000
#define CACUS_CALLBACK_DEBUGALL   0x1FFFFFFF


#define CDBG_ERROR             -1
#define CDBG_OK                 0


typedef void (*CACUS_DEBUG_CALLBACK_FUNC)(const char*,int);

/* CDbg_RegisterCallback( &ExampleCallback, CACUS_CALLBACK_ALL)
void ExampleCallback( const char* Message, int MessageFlags)
{
	Log( Message);
}
*/

/* CDbg_RegisterCallback( &ExceptionCallback, CACUS_CALLBACK_EXCEPTION)
void ExceptionCallback( const char* Message, int MessageFlags)
{
	throw Message; //This will stop other callbacks! Make sure to register last
}
*/

extern "C"
{
	CACUS_API int  CDbg_RegisterCallback( CACUS_DEBUG_CALLBACK_FUNC Callback, int CallbackFlags=CACUS_CALLBACK_ALL, int Slot=-1); //Can be used to change slot
	CACUS_API int  CDbg_UnregisterCallback( CACUS_DEBUG_CALLBACK_FUNC Callback);
	CACUS_API int  CDbg_SlotCallback( CACUS_DEBUG_CALLBACK_FUNC Callback); //Returns CDBG_ERROR/INDEX_NONE if not registered
	CACUS_API void CDbg_UnlockCallback(); //Should never be used, releases thread lock in callback
};


#endif