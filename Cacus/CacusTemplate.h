#ifndef USES_CACUS_TEMPLATE
#define USES_CACUS_TEMPLATE

#include "CacusPlatform.h"
#include "Atomics.h"

#ifndef ARRAY_COUNT
	#define ARRAY_COUNT( array ) \
		( sizeof(array) / sizeof((array)[0]) )
#endif

/**
* gcc3 thinks &((myclass*)NULL)->member is an invalid use of the offsetof
* macro. This is a broken heuristic in the compiler and the workaround is
* to use a non-zero offset.
*/
#ifdef __GNUC__
	#define CSTRUCT_BASE 0x1
#else
	#define CSTRUCT_BASE NULL
#endif
#define CSTRUCT_MEMBER(struc,member)  ((struc*)CSTRUCT_BASE)->member
#define CSTRUCT_OFFSET(struc,member)	( ( (uint32)&CSTRUCT_MEMBER(struc,member) ) - CSTRUCT_BASE )
#define CSTRUCT_ARRAY_COUNT(struc,member)     ARRAY_COUNT( CSTRUCT_MEMBER(struc,member) )


#ifndef NO_CPP11_TEMPLATES
	//User defined literal
	inline constexpr unsigned char operator "" _UC( unsigned long long int arg ) noexcept
	{
		return static_cast< unsigned char >( arg );
	}
#endif

//User defined number/data logic
template< class T > inline T Abs( const T A )
{
	return (A>=(T)0) ? A : -A;
}
template< class T > inline T Sgn( const T A )
{
	return (A>0) ? 1 : ((A<0) ? -1 : 0);
}
template< class T > inline T Max( const T A, const T B )
{
	return (A>=B) ? A : B;
}
template< class T > inline T Min( const T A, const T B )
{
	return (A<=B) ? A : B;
}
template< class T > inline T Clamp( const T X, const T Min, const T Max )
{
	return X<Min ? Min : X<Max ? X : Max;
}
template< class T > inline void Swap( T& A, T& B )
{
	const T Temp = A;
	A = B;
	B = Temp;
}
template< class T > inline void Exchange( T& A, T& B )
{
	Swap(A, B);
}

template< typename TOUT=void, typename TIN > inline TOUT* AddressOffset( TIN* Address, int_p Offset)
{
	return (TOUT*)((uint8*)Address + Offset);
}

template< typename T=void > inline T* AddressAlign( T* Addr, const size_t Align)
{
	if ( Align > 1 )
	{
		const size_t Mask = Align-1;
		Addr = (T*)(((size_t)Addr + Mask) & (~Mask));
	}
	return Addr;
}

//=========================================================
// Bit metas
template<uint32 N, uint32 Bit=31> struct TBits
{
	static const uint32 Counter = (N>>(Bit-1UL)) ? Bit : TBits<N,Bit-1>::Counter;
	static const uint32 Mask = (1 << Counter) - 1;
	static const uint32 MostSignificant = 1 << (Counter-1);
};
template<uint32 N> struct TBits<N,0>
{
	static const uint32 Counter = 0;
	static const uint32 Mask = 0;
	static const uint32 MostSignificant = 0;
};
static_assert( TBits<20>::Counter == 5, "EEOR1");
static_assert( TBits<20>::Mask == 0b11111, "EEOR2");
static_assert( TBits<20>::MostSignificant == 0b10000, "EEOR3");


//=========================================================
// Circular array - FIFO order
template<typename ArrayType, uint32 ArrayDim> class TCircularArray
{
	uint32 Position; //Inaccesible
	ArrayType Array[ArrayDim]; //Inaccesible
public:
	void AddNew( ArrayType NewElement)                { Array[ (--Position) % ArrayDim ] = NewElement; }

	ArrayType&       operator[](int32 Offset)         { return Array[ (Position+Offset) % ArrayDim ]; }
	const ArrayType& operator[](int32 Offset) const   { return Array[ (Position+Offset) % ArrayDim ]; }

	static constexpr uint32 Size()                           { return ArrayDim; }
};


//=========================================================
// Loop based Linked list object handler
// Prevents dangerous, unneeded stack allocations at the cost of unordered destruction
template< class T > inline void LinkedListDelete( T*& List)
{
	while ( List )
	{
		T* Next = List->Next;
		List->Next = nullptr;
		delete List;
		List = Next;
	}
}

//=========================================================
// One-line deleter
struct TDeleteOp
{
	template< typename T > FORCEINLINE bool operator==( T*& Other)
	{
		if ( Other )
		{
			delete Other;
			Other = nullptr;
			return true;
		}
		return false;
	}
};
#define CDelete TDeleteOp() ==


//=========================================================
// Simple array container, good for implicit Malloc+Free within scope
//
class CACUS_API CFixedArray
{
protected:
	void* Data;
	uint32 Num;

	CFixedArray()                                         : Data(nullptr), Num(0) {}
public:
	uint32 Size() const                                   { return Num; }
	void* GetData()                                       { return Data; }
	void Setup( uint32 NewNum, size_t ElementSize);

	~CFixedArray()                                        { if ( Data ) CFree(Data); Data = nullptr; }

	bool operator==(const CFixedArray& Other) const       { return Num==Other.Num && Data==Other.Data; }
	bool operator!=(const CFixedArray& Other) const       { return Num!=Other.Num || Data!=Other.Data; }
};

template< class T > class TFixedArray : public CFixedArray
{
public:
	TFixedArray()                                         : CFixedArray() {}
	TFixedArray(uint32 InSize)                            : CFixedArray() { Setup(InSize); }

	#ifndef NO_MOVE_CONSTRUCTORS
	TFixedArray( TFixedArray<T>&& Other) noexcept //Move constructor, used for functions returning this object
	{
		Data = Other.Data;
		Num = Other.Num;
		Other.Data = nullptr;
		Other.Num = 0;
	}
	#endif

	void Setup( uint32 NewNum)                            { CFixedArray::Setup( NewNum, sizeof(T) ); }
	operator T*()                                         { return (T*)Data; }
	operator const T*() const                             { return (const T*)Data; }
	static constexpr size_t ElementSize()                 { return sizeof(T); }
};


//=========================================================
// Multi-shape linked list, the user must know what kind of
// primitive the tagged node is made of.

class CTaggedNode
{
protected:
	CTaggedNode** Container;
	CTaggedNode* Prev;
	CTaggedNode* Next;
	uint64 Tag;

	//Auto attach/detach
	CTaggedNode( CTaggedNode*& InContainer);
	~CTaggedNode();
public:
	CTaggedNode* FindTag( uint64 Find);
};

template<typename T> class TTaggedNode : public CTaggedNode
{
public:
	typedef T Primitive;
	uint64 Cycles; //Cycle counter
	Primitive Data;

	TTaggedNode( CTaggedNode*& InContainer) : CTaggedNode(InContainer) {}
	~TTaggedNode() {}
};

//=========================================================
//STL expanders
#ifdef _VECTOR_
template< class T > class TMasterObjectArray
{
public:
	std::vector<T> List;
	volatile int32 Lock;

	TMasterObjectArray()
		: List(), Lock(0) {}

	bool operator!=(const TMasterObjectArray<T>& Other)
	{
		return true; //STL isn't exactly helping here
	}

	uint32 Add( T Elem)
	{
		CSleepLock SL( &Lock);
		for ( uint32 i=0 ; i<List.size() ; i++ )
			if ( List[i] == Elem )
				return i;
		List.push_back( Elem);
		return List.size()-1;
	}

	bool Remove( T Elem)
	{
		CSleepLock SL( &Lock);
		for ( uint32 i=0 ; i<List.size() ; i++ )
			if ( List[i] == Elem )
			{
				List.erase( List.begin() + i);
				return true;
			}
		return false;
	}

	bool Has( T Elem)
	{
		CSleepLock SL( &Lock, 0);
		for ( uint32 i=0 ; i<List.size() ; i++ )
			if ( List[i] == Elem )
				return true;
		return false;
	}

	void Empty( bool bDestroy=true)
	{
		CSleepLock SL( &Lock, 0);
		if ( bDestroy )
			for ( uint32 i=0 ; i<List.size() ; i++ )
				if ( List[i] )
					delete List[i];
		List.clear();
	}
};
#endif


#endif