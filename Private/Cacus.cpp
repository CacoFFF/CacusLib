
#include "CacusString.h"
#include "DebugCallback.h"
#include "CacusTemplate.h"

int32 volatile COpenThreads = 0;

#ifdef CACUS_OLD_CRT
	#include "../OldCRT/API_MSC.h"
#endif




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
