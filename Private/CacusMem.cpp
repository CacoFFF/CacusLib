/*=============================================================================
	CacusMem.cpp
	Author: Fernando Velázquez

	Memory containers.
=============================================================================*/


#include "CacusLibPrivate.h"

#include "CacusMem.h"
#include "TCharBuffer.h"


// This is not thread safe.
static TChar8Buffer<256> ThrowLocal;




//========= Extendable Stack: PushBlock - begin ==========//
//
// Attaches an additional memory block to this extendable stack.
// The resulting block may be an already existing unused block.
// 
// Parameters:
// - InSize: minimum allowed size in bytes for the new memory block.
//
void CMemExStack::PushBlock( size_t InSize)
{
	// Locate a suitable unused memory block
	MemBlock* NewTopBlock = UnusedBlock;
	while ( NewTopBlock && NewTopBlock->Size < InSize )
		NewTopBlock = NewTopBlock->Next;

	if ( !NewTopBlock )
	{
		// Create one if needed
		NewTopBlock = (MemBlock*)CMalloc(sizeof(MemBlock) + InSize);
		if ( !NewTopBlock )
		{
			sprintf( *ThrowLocal, "CMemExStack -> no memory for TopBlock - Requested %i bytes", (int)InSize );
			throw *ThrowLocal;
		}
		NewTopBlock->Size = InSize;
	}
	NewTopBlock->Next = TopBlock;
	Top = NewTopBlock->Data;
	End = Top + InSize;
}
//========= Extendable Stack: PushBlock - end ==========//



struct _cb_header_
{
	int32 Lock;
	size_t BufferSize;
	size_t CurPos;
};

CCircularBuffer* CircularAllocate( size_t BufferSize)
{
	_cb_header_* Buf = (_cb_header_*)CMalloc( BufferSize + sizeof(_cb_header_) );
	Buf->Lock = 0;
	Buf->BufferSize = BufferSize;
	Buf->CurPos = 0;
	return (CCircularBuffer*)Buf;
}

void CircularFree( CCircularBuffer* Buffer)
{
	if ( Buffer )
		CFree( Buffer);
}