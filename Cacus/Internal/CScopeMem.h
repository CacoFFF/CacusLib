/*=============================================================================
	Internal/CScopedMem.h:
	Inlinable scoped memory util
	Author: Fernando Velázquez

	Included by CacusBase.h
	This class provides an abstraction layer over simple memory allocations
	that use CMalloc, CFree.
=============================================================================*/


//
// CScopeMem class declaration
//
class CScopeMem
{
protected:
	void*  Data;
	size_t Size;

public:
	CScopeMem();
	CScopeMem(ENoInit);
	CScopeMem( size_t InSize);
	CScopeMem( CScopeMem&& Move) noexcept;
	CScopeMem( const CScopeMem& Copy);
	~CScopeMem();

	operator bool();
	CScopeMem& operator=(CScopeMem&& Move);
	CScopeMem& operator=(const CScopeMem& Copy);

	void*       GetData();
	const void* GetData() const;
	size_t      GetSize() const;
	template <typename T> T* GetArray();

	void* Detach();
	void  Empty();
};


/*-----------------------------------------------------------------------------
	CScopeMem implementation.
-----------------------------------------------------------------------------*/

//
// Constructors
//
inline CScopeMem::CScopeMem()
	: Data(nullptr)
	, Size(0)
{
}

inline CScopeMem::CScopeMem(ENoInit)
{
}

inline CScopeMem::CScopeMem( size_t InSize)
{
	if ( InSize != 0 )
	{
		Data = CMalloc(InSize);
		if ( Data == nullptr )
		{
		#if defined(_EXCEPTION_)
			throw std::bad_alloc();
		#endif
		}
	}
	else
		Data = nullptr;
	Size = InSize;
}

inline CScopeMem::CScopeMem( CScopeMem&& Move) noexcept
	: Data(Move.Data)
	, Size(Move.Size)
{
	Move.Data = nullptr;
	Move.Size = 0;
}

inline CScopeMem::CScopeMem( const CScopeMem& Copy)
	: CScopeMem(Copy.Size)
{
	if ( Data != nullptr )
		CMemcpy(Data, Copy.Data, Copy.Size);
}


//
// Destructor
//
inline CScopeMem::~CScopeMem()
{
	Empty();
}


//
// Logic
//
inline CScopeMem::operator bool()
{
	return Data != nullptr;
}

inline CScopeMem& CScopeMem::operator=( CScopeMem&& Move)
{
	if ( Data )
		CFree(Data);
	Data = Move.Data;
	Size = Move.Size;
	Move.Data = nullptr;
	Move.Size = 0;
	return *this;
}

inline CScopeMem& CScopeMem::operator=( const CScopeMem& Copy)
{
	Empty();
	if ( Copy.Size && Copy.Data )
	{
		new(this,E_InPlace) CScopeMem(Copy.Size);
		CMemcpy(Data, Copy.Data, Copy.Size);
	}
	return *this;
}


//
// Access
//
inline void* CScopeMem::GetData()
{
	return Data;
}

inline const void* CScopeMem::GetData() const
{
	return Data;
}

inline size_t CScopeMem::GetSize() const
{
	return Size;
}

template<typename T>
inline T* CScopeMem::GetArray()
{
	return (T*)Data;
}


//
// Actions
//
inline void* CScopeMem::Detach()
{
	void* Result = Data;
	Data = nullptr;
	Size = 0;
	return Result;
}

inline void CScopeMem::Empty()
{
	if ( Data )
	{
		CFree(Data);
		Data = nullptr;
		Size = 0;
	}
}
