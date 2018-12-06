
#include "CacusMem.h"

struct _cb_header_
{
	int32 Lock;
	size_t BufferSize;
	size_t CurPos;
};

CCircularBuffer* CircularAllocate( size_t BufferSize)
{
	_cb_header_* Buf = (_cb_header_*)CMalloc( BufferSize);
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
