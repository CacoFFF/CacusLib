#pragma once

#include "CacusPlatform.h"
#include "CacusTemplate.h"
#include "Atomics.h"

//Tags
enum { EALIGN_Byte  = 1 };
enum { EALIGN_Word = 2 };
enum { EALIGN_DWord = 4 };
enum { EALIGN_QWord = 8 };
enum { EALIGN_XMMWord = 16 };
enum { EALIGN_AVXWord = 32 };

//Align options
enum EAlignOptions
{
	EALIGN_1  = EALIGN_Byte,
	EALIGN_2  = EALIGN_Word,
	EALIGN_4  = EALIGN_DWord,
	EALIGN_8  = EALIGN_QWord,
	EALIGN_16 = EALIGN_XMMWord,
	EALIGN_32 = EALIGN_AVXWord,
	EALIGN_PLATFORM_PTR = sizeof(void*),
};

template <typename T> EAlignOptions GetAlignmentType()
{
	return (EAlignOptions)sizeof(T);
}

//*****************************************************//
// Circular Buffer
//
// Provides a single data buffer that can be fragmented
// on demand, while trying to preserve previously
// requested data for as long as possible.
//

class CCircularBuffer;
extern "C"
{
	CACUS_API CCircularBuffer* CircularAllocate( size_t BufferSize);
	CACUS_API void CircularFree( CCircularBuffer* Buffer);
};

class CCircularBuffer
{
	volatile int32 Lock;
	size_t BufferSize;
	size_t CurPos;
	uint8 Data[1];

	CCircularBuffer() {};
	~CCircularBuffer() {};

public:
	size_t Size()                 { return BufferSize; }

	template<enum EAlignOptions Align=EALIGN_PLATFORM_PTR,bool bThrow=true> uint8* Request( size_t Amount)
	{
		if ( Amount >= BufferSize ) //Using >= instead of > in case base data buffer needs alignment too
		{
			if ( bThrow )
				throw "TCircularBuffer cannot be requested more than its Size";
			return nullptr;
		}

		CSpinLock SL(&Lock);
		uint8* Start = AddressAlign( Data + CurPos, Align);
		if ( Start + Amount > Data + BufferSize )
			Start = AddressAlign(Data, Align);

		CurPos = (size_t)(Start-Data) + Amount;
		return Start;
/*
		size_t Start = CurPos;
		
		if ( Align > 1 )
		{
			const uint32 Mask = Align-1;
			Start = (Start + Mask) & (~Mask);
		}

		if ( Start + Amount >= BufferSize )
			Start = 0;

		CurPos = Start+Amount;
		return Data + Start;*/
	}
};


