#pragma once

#include "CacusPlatform.h"
#include "CacusTemplate.h"
#include "DebugCallback.h"
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
// Extensible stack
//
// On-demand data block allocation system for fast single
// threaded processing.
// 
// Based on Unreal Engine's FMemStack
//

class CMemExStack
{
	struct MemBlock
	{
		MemBlock* Next;
		size_t    Size;
		uint8     Data[1];
	};

	size_t DefaultSize;
	MemBlock* TopBlock;
	MemBlock* UnusedBlock;
	uint8* Top; // Top <= End
	uint8* End;

public:
	CMemExStack( size_t InDefaultSize=4096);
	~CMemExStack();
	uint8* PushBytes( size_t InSize, size_t InAlign);

	friend void* operator new( size_t Size, CMemExStack& Mem, size_t Count=1, size_t Align=EALIGN_PLATFORM_PTR);
	friend void operator delete( void*, CMemExStack&, size_t, size_t) {};
private:
	void CACUS_API PushBlock( size_t InSize);
};


inline CMemExStack::CMemExStack( size_t InDefaultSize)
	: DefaultSize(InDefaultSize)
	, TopBlock(nullptr)
	, UnusedBlock(nullptr)
	, Top(nullptr)
	, End(nullptr)
{
	PushBlock(InDefaultSize);
}

inline CMemExStack::~CMemExStack()
{
	LinkedListFree(TopBlock);
	LinkedListFree(UnusedBlock);
}

inline uint8* CMemExStack::PushBytes( size_t InSize, size_t InAlign=EALIGN_PLATFORM_PTR)
{
	uint8* Result = AddressAlign(Top, InAlign);
	Top = Result + InSize;
	if ( Top > End )
	{
		PushBlock( Max(InSize,DefaultSize) );
		Result = AddressAlign(Top, InAlign);
		Top = Result + InSize;
	}
	return Result;
}

inline void* operator new( size_t Size, CMemExStack& Mem, size_t Count, size_t Align )
{
	return Mem.PushBytes( Size*Count, Align );
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
			{
				char Buffer[96];
				sprintf( Buffer, "TCircularBuffer: Requested [%i/%i] bytes", (int)Amount, (int)BufferSize);
				DebugCallback( Buffer, CACUS_CALLBACK_MEMORY|CACUS_CALLBACK_EXCEPTION);
			}
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


