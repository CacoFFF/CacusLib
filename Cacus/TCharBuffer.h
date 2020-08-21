/*=============================================================================
	TCharBuffer.h:

	Templated char buffers with c++ styled interface.

	Author: Fernando Velázquez.
=============================================================================*/
#ifndef USES_CACUS_TCHARBUF
#define USES_CACUS_TCHARBUF

#include "CacusString.h"

template< size_t N, typename CHAR> class TCharBuffer
{
	CHAR Buffer[N];
public:
	//******************
	// Constructors
	TCharBuffer()
	{
		Buffer[0] = '\0';
	}
	template<typename CSRC> TCharBuffer( const CSRC* Src)
	{
		CStrcpy_s(Buffer,N,Src);
	}

	//******************
	// Pointer to buffer/char

	CHAR* operator*()                             { return Buffer; }
	const CHAR* operator*() const                 { return Buffer; }
	CHAR& operator[]      ( size_t i)             { return Buffer[i]; }
	const CHAR&  operator[]( size_t i) const      { return Buffer[i]; }

	//******************
	// Asignment
	template<typename CSRC> TCharBuffer<N,CHAR>& operator=(const CSRC* Src)
	{
		CStrcpy_s(Buffer,N,Src); return *this;
	}

	TCharBuffer<N,CHAR>& operator+=(const CHAR* Append)
	{
		//Compiler optimization will cause failure to concatenate strings if self-concatenation occurs (or similar)
		if ( (Append >= &Buffer[0]) && (Append < &Buffer[N]) )
		{
			TCharBuffer<N,CHAR> TmpBuf = Append;
			CStrcat_s(Buffer,N,*TmpBuf);
		}
		else
			CStrcat_s(Buffer,N,Append);
		return *this;
	}
	

	//******************
	// Comparison
	bool operator==(const CHAR* Other) const      { return CStrcmp(Buffer,Other) == 0; }
	bool operator!=(const CHAR* Other) const      { return CStrcmp(Buffer,Other) != 0; }
	bool operator< (const CHAR* Other) const      { return CStrcmp(Buffer,Other) < 0; }
	bool operator> (const CHAR* Other) const      { return CStrcmp(Buffer,Other) > 0; }

	template< size_t N2> bool operator==(const TCharBuffer<N2,CHAR>& Other) const { return CStrcmp(Buffer,*Other) == 0; }
	template< size_t N2> bool operator!=(const TCharBuffer<N2,CHAR>& Other) const { return CStrcmp(Buffer,*Other) != 0; }
	template< size_t N2> bool operator< (const TCharBuffer<N2,CHAR>& Other) const { return CStrcmp(Buffer,*Other) < 0; }
	template< size_t N2> bool operator> (const TCharBuffer<N2,CHAR>& Other) const { return CStrcmp(Buffer,*Other) > 0; }

	//******************
	// Utils
	static constexpr size_t Size()
	{
		return N;
	}
	size_t Len() const
	{
		return CStrlen(Buffer);
	}

	size_t InStr( CHAR C, size_t Offset=0) const
	{
		CHAR* Found = CStrchr( Buffer + Offset, C);
		if ( Found )
			return Found - Buffer;
		return INDEX_NONE;
	}

	void ToLower()
	{
		TransformLowerCase( Buffer);
	}

	void ToUpper()
	{
		TransformUpperCase( Buffer);
	}

	void Fix()
	{
		Buffer[N-1] = '\0';
	}

#ifdef _INC_STDIO
	void print() const
	{
		if ( sizeof(CHAR) == 1 )
			printf( (const char*)Buffer);
		else
			wprintf(Wide());
	}
#endif

	// wchar_t converter
	const wchar_t* Wide() const
	{
		if ( sizeof(CHAR) == sizeof(wchar_t) ) //Constant conditional
			return (const wchar_t*)Buffer;
		else
		{
			wchar_t* wBuffer = CharBuffer<wchar_t>(N);
			CStrcpy_s( wBuffer, N, Buffer);
			return (const wchar_t*)wBuffer;
		}
	}

public:
	class TWriter
	{
	public:
		TWriter()
			: Pos(0)
		{}

		TWriter& operator<<( const CHAR* Text)
		{
			CHAR* BufPtr = *Buffer;
			while ( *Text && (Pos < N-1) )
				BufPtr[Pos++] = *Text++;
			BufPtr[Pos] = '\0';
			return *this;
		}
		// TODO: Add other types for <<

		size_t GetPos() const                   { return Pos; }
		TCharBuffer<N,CHAR>& GetBuffer()		{ return Buffer; }

	private:
		size_t Pos;
		TCharBuffer<N,CHAR> Buffer;
	};
};
#ifndef NO_CPP11_TEMPLATES
	template <size_t N> using TChar8Buffer    = TCharBuffer<N, char>;
	template <size_t N> using TChar16Buffer   = TCharBuffer<N, char16>;
	template <size_t N> using TChar32Buffer   = TCharBuffer<N, char32>;
	template <size_t N> using TCharWideBuffer = TCharBuffer<N, wchar_t>;
#endif


#endif