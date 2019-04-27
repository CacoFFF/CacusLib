
#include "CacusGlobals.h"
#include "CacusOutputDevice.h"
#include "CacusString.h"
#include "StackUnwinder.h"
#include "DebugCallback.h"
#include "CacusTemplate.h"

int32 volatile COpenThreads = 0;


//
// Unwind the stack.
//
void CUnwindf( const char* Msg )
{
	CCriticalError( nullptr);

/*	static int32 Count=0;
	if( Count++ )
		strncat( GErrorHist, TEXT(" <- "), ARRAY_COUNT(GErrorHist) );
	strncat( GErrorHist, Msg, ARRAY_COUNT(GErrorHist) );*/

	DebugCallback( Msg, CACUS_CALLBACK_THREAD);
}

void CCriticalError( const char* Error)
{
	thread_local int CriticalError = 0;
	if( !CriticalError++ )
	{
		DebugCallback("\r\n=== Critical Error ===", CACUS_CALLBACK_THREAD);
		if ( Error )
			DebugCallback( Error, CACUS_CALLBACK_THREAD);
		DebugCallback( "== History:", CACUS_CALLBACK_THREAD | CACUS_CALLBACK_EXCEPTION);
		if ( Error ) //Not called from Unwindf
			throw 1;
	}
}

void CFailAssert( const char* Error, const char* File, int32 Line)
{
	char Buffer[512];
	sprintf( Buffer, "Assertion failed: [%s] at %s line %i", Error, File, Line);
	CCriticalError( Buffer);
}

void CFixedArray::Setup( uint32 NewNum, size_t ElementSize)
{
	if ( NewNum != Num )
	{
		void* OldData = Data;
		size_t OldAllocSize = Num*ElementSize;

		size_t AllocSize = NewNum*ElementSize;
		if ( AllocSize )
		{
			Data = CMalloc( AllocSize);
			memset( Data, 0, AllocSize);

			if ( OldAllocSize && AllocSize )
				CMemcpy( Data, OldData, Min(AllocSize,OldAllocSize) );
		}
		Num = NewNum;
		if ( OldData )
			CFree(OldData);
	}
}

static volatile uint64 NodeTag = 0;
CTaggedNode::CTaggedNode( CTaggedNode*& InContainer)
	: Container(&InContainer)
	, Prev(nullptr)
	, Next(InContainer)
	, Tag( ++NodeTag )
{
	Next->Prev = this; 
}

CTaggedNode::~CTaggedNode()
{
	if (!Container)
		return;

	if (Prev)
		Prev->Next = Next;
	else if ( *Container == this )
		*Container = Next;
//	else
//		throw "Error in tagged nodes"

	if (Next)
		Next->Prev = Prev;

	DebugCallback( "Tagged node destruct ok", CACUS_CALLBACK_THREAD | CACUS_CALLBACK_DEBUGONLY);
}

CTaggedNode* CTaggedNode::FindTag(uint64 Find)
{
	for ( CTaggedNode* Link=this ; Link ; Link=Link->Next )
		if ( Link->Tag == Find )
			return Link;
	return nullptr;
}
