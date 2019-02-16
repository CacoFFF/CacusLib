/*=============================================================================
	CacusString.h:

	C styled string library for all character sizes.
	Provides uniform method names for simplicity.

	Author: Fernando Velázquez.
=============================================================================*/

#ifndef USES_CACUS_STRING
#define USES_CACUS_STRING

#include "CacusPlatform.h"

//*******************************************************************
// TYPE INCLUDES

#ifndef char16
	#define char16 char16_t
#endif
#ifndef char32
	#define char32 char32_t
#endif


//*******************************************************************
// TYPE TEMPLATES

enum ECharType
{
	CHTYPE_Upper   = 0x0001,
	CHTYPE_Lower   = 0x0002,
	CHTYPE_Digit   = 0x0004,
	CHTYPE_Punct   = 0x0010,
	CHTYPE_Cntrl   = 0x0020,
	CHTYPE_TokOp   = 0x0040, //Token splitter (operators, signs, etc)
	CHTYPE_TokSp   = 0x0080, //Token splitter (separators)
	CHTYPE_TokSc   = 0x0100, //Single character token {}[]()

	CHTYPE_Alpha   = CHTYPE_Upper | CHTYPE_Lower,
	CHTYPE_AlNum   = CHTYPE_Alpha | CHTYPE_Digit,
};


//Compiler safety enforcer
template<typename C> inline constexpr bool CheckCharType()           { return false; } 
template<> inline constexpr bool CheckCharType<char>()               { return true; }
template<> inline constexpr bool CheckCharType<char16>()             { return true; }
template<> inline constexpr bool CheckCharType<char32>()             { return true; }
template<> inline constexpr bool CheckCharType<wchar_t>()            { return true; }

#ifndef NO_CPP11_TEMPLATES
	//Constant expression string length (cannot use recursive versions because MSVC isn't fully compliant)
	template< typename CHAR , size_t ArSize > constexpr FORCEINLINE size_t _len( const CHAR (&Str)[ArSize])
	{
		static_assert( CheckCharType<CHAR>(),"_len: Invalid character type");
		return ArSize-1;
	}
#endif


//*******************************************************************
// C STRING REIMPLEMENTATIONS

extern "C"
{
	CACUS_API uint32 CGetCharType( uint32 ch);
	CACUS_API bool CIsDigit( uint32 ch);

	CACUS_API int CStrcpy8_s ( char*   Dest, uint32 DestChars, const char*   Src);
	CACUS_API int CStrcpy16_s( char16* Dest, uint32 DestChars, const char16* Src);
	CACUS_API int CStrcpy32_s( char32* Dest, uint32 DestChars, const char32* Src);
	CACUS_API int CStrcpy_g_s( void* Dest, uint32 DestBytes, uint32 DestChars, const void* Src, uint32 SrcBytes); //Handles char <-> wide conversions

	CACUS_API char*   CStrcat8_s ( char*   Dest, size_t DestSize, const char*   Src);
	CACUS_API char16* CStrcat16_s( char16* Dest, size_t DestSize, const char16* Src);
	CACUS_API char32* CStrcat32_s( char32* Dest, size_t DestSize, const char32* Src);

	CACUS_API size_t CStrlen8 ( const char*   Str);
	CACUS_API size_t CStrlen16( const char16* Str);
	CACUS_API size_t CStrlen32( const char32* Str);

	CACUS_API char*   CStrchr8 ( const char*   Str, int32 Find);
	CACUS_API char16* CStrchr16( const char16* Str, int32 Find);
	CACUS_API char32* CStrchr32( const char32* Str, int32 Find);

	CACUS_API char*   CStrstr8 ( const char*   Str, const char*   Find);
	CACUS_API char16* CStrstr16( const char16* Str, const char16* Find);
	CACUS_API char32* CStrstr32( const char32* Str, const char32* Find);

	CACUS_API int CStrcmp8 ( const char*   S1, const char*   S2);
	CACUS_API int CStrcmp16( const char16* S1, const char16* S2);
	CACUS_API int CStrcmp32( const char32* S1, const char32* S2);

	CACUS_API int CStrncmp8 ( const char*   S1, const char*   S2, size_t cmplen);
	CACUS_API int CStrncmp16( const char16* S1, const char16* S2, size_t cmplen);
	CACUS_API int CStrncmp32( const char32* S1, const char32* S2, size_t cmplen);

	CACUS_API char*    VARARGS CSprintf( const char* fmt, ...); //Print into circular buffer
	CACUS_API wchar_t* VARARGS CWSprintf( const wchar_t* fmt, ...); //Print into circular buffer
}

template< typename CHARDEST , typename CHARSRC > int FORCEINLINE CStrcpy_s( CHARDEST* Dest, uint32 DestSize, const CHARSRC* Src)
{
	if      ( sizeof(CHARDEST)==1 && sizeof(CHARSRC)==1 ) return CStrcpy8_s ( (char*)  Dest, DestSize, (const char*)   Src);
	else if ( sizeof(CHARDEST)==2 && sizeof(CHARSRC)==2 ) return CStrcpy16_s( (char16*)Dest, DestSize, (const char16*) Src);
	else if ( sizeof(CHARDEST)==4 && sizeof(CHARSRC)==4 ) return CStrcpy32_s( (char32*)Dest, DestSize, (const char32*) Src);
	else                                                  return CStrcpy_g_s( Dest, sizeof(CHARDEST), DestSize, Src, sizeof(CHARSRC));
}

#ifndef NO_CPP11_TEMPLATES
	template< typename CHARDEST , typename CHARSRC , uint32 DestChars > int FORCEINLINE CStrcpy_s( CHARDEST (&Dest)[DestChars], const CHARSRC* Src)
	{
		return CStrcpy_s<CHARDEST,CHARSRC>( Dest, DestChars, Src);
	}
#endif

template < typename CHAR > FORCEINLINE CHAR* CStrcat_s( CHAR* Dest, size_t DestSize, const CHAR* Src)
{
	if ( sizeof(CHAR) == 1 ) return (CHAR*)CStrcat8_s ( (char*)  Dest, DestSize, (const char*)  Src);
	if ( sizeof(CHAR) == 2 ) return (CHAR*)CStrcat16_s( (char16*)Dest, DestSize, (const char16*)Src);
	if ( sizeof(CHAR) == 4 ) return (CHAR*)CStrcat32_s( (char32*)Dest, DestSize, (const char32*)Src);
}

template < typename CHAR > FORCEINLINE size_t CStrlen( const CHAR* Str)
{
	if ( sizeof(CHAR) == 1 ) return CStrlen8 ( (const char*)  Str);
	if ( sizeof(CHAR) == 2 ) return CStrlen16( (const char16*)Str);
	if ( sizeof(CHAR) == 4 ) return CStrlen32( (const char32*)Str);
}
template < typename CHAR > FORCEINLINE CHAR* CStrchr( const CHAR* Str, int32 Find)
{
	if ( sizeof(CHAR) == 1 ) return (CHAR*)CStrchr8 ( (const char*)  Str, Find);
	if ( sizeof(CHAR) == 2 ) return (CHAR*)CStrchr16( (const char16*)Str, Find);
	if ( sizeof(CHAR) == 4 ) return (CHAR*)CStrchr32( (const char32*)Str, Find);
}
template < typename CHAR > FORCEINLINE CHAR* CStrstr( const CHAR* Str, const CHAR* Find)
{
	if ( sizeof(CHAR) == 1 ) return (CHAR*)CStrstr8 ( (const char*)  Str, (const char*)  Find);
	if ( sizeof(CHAR) == 2 ) return (CHAR*)CStrstr16( (const char16*)Str, (const char16*)Find);
	if ( sizeof(CHAR) == 4 ) return (CHAR*)CStrstr32( (const char32*)Str, (const char32*)Find);
}
template < typename CHAR > FORCEINLINE int CStrcmp( const CHAR* S1, const CHAR* S2)
{
	if ( sizeof(CHAR) == 1 ) return CStrcmp8 ( (const char*)  S1, (const char*)  S2);
	if ( sizeof(CHAR) == 2 ) return CStrcmp16( (const char16*)S1, (const char16*)S2);
	if ( sizeof(CHAR) == 4 ) return CStrcmp32( (const char32*)S1, (const char32*)S2);
}
template < typename CHAR > FORCEINLINE int CStrncmp( const CHAR* S1, const CHAR* S2, size_t cmplen)
{
	if ( sizeof(CHAR) == 1 ) return CStrncmp8 ( (const char*)  S1, (const char*)  S2, cmplen);
	if ( sizeof(CHAR) == 2 ) return CStrncmp16( (const char16*)S1, (const char16*)S2, cmplen);
	if ( sizeof(CHAR) == 4 ) return CStrncmp32( (const char32*)S1, (const char32*)S2, cmplen);
}
#ifndef NO_CPP11_TEMPLATES
	template < typename CHAR, size_t cmpsize > FORCEINLINE int CStrncmp( const CHAR* S1, const CHAR(&S2)[cmpsize])
	{
		return CStrncmp( S1, S2, cmpsize-1);
	}
#endif


//Cleans up a string buffer, useful for safety purposes
template<typename C> FORCEINLINE void CStrZero( C* Str)
{
	while ( *Str ) *Str++='\0';
}

//Turn an existing buffer into lower case
template<typename C> FORCEINLINE void TransformLowerCase( C* Str)
{
	for ( ; *Str ; Str++ )	if ( CGetCharType(*Str) & CHTYPE_Upper )	*Str += ('a' - 'A');
}

//Turn an existing buffer into upper case
template<typename C> FORCEINLINE void TransformUpperCase( C* Str)
{
	for ( ; *Str ; Str++ )	if ( CGetCharType(*Str) & CHTYPE_Lower )	*Str -= ('a' - 'A');
}


//*******************************************************************
// STRING UTILS


//PORT TO MULTITYPE!
CACUS_API char* ParseToken( const char* Text, size_t* ModifyPos=nullptr);
extern "C"
{
	//Empty returns false
	CACUS_API bool IsAlphaNumeric8 ( const char*   Str);
	CACUS_API bool IsAlphaNumeric16( const char16* Str);
	CACUS_API bool IsAlphaNumeric32( const char32* Str);

	//Empty returns false
	CACUS_API bool IsNumeric8 ( const char*   Str);
	CACUS_API bool IsNumeric16( const char16* Str);
	CACUS_API bool IsNumeric32( const char32* Str);

	//Unix doesn't use \ backslash
#if _WINDOWS
	#define OSpath(Path) Path
#else
	CACUS_API const char* OSpath  ( const char* Path);
#endif

	//Copies into internal static buffer (no need to destroy)
	CACUS_API char*   CopyToBuffer8 ( const char*   Src, size_t Len=MAXINT);
	CACUS_API char16* CopyToBuffer16( const char16* Src, size_t Len=MAXINT);
	CACUS_API char32* CopyToBuffer32( const char32* Src, size_t Len=MAXINT);

//	CACUS_API char* CLowerAnsi( const char* Src);
};

template < typename CHAR > FORCEINLINE bool IsAlphaNumeric( const CHAR* Str)
{
	if ( sizeof(CHAR) == 1 ) return IsAlphaNumeric8(  (const char*)  Str);
	if ( sizeof(CHAR) == 2 ) return IsAlphaNumeric16( (const char16*)Str);
	if ( sizeof(CHAR) == 4 ) return IsAlphaNumeric32( (const char32*)Str);
}
template < typename CHAR > FORCEINLINE bool IsNumeric( const CHAR* Str)
{
	if ( sizeof(CHAR) == 1 ) return IsNumeric8 ( (const char*)  Str);
	if ( sizeof(CHAR) == 2 ) return IsNumeric16( (const char16*)Str);
	if ( sizeof(CHAR) == 4 ) return IsNumeric32( (const char32*)Str);
}
template < typename CHAR > FORCEINLINE CHAR* CopyToBuffer( const CHAR* Src, size_t Len=MAXINT)
{
	if ( sizeof(CHAR) == 1 ) return (CHAR*)CopyToBuffer8 ( (const char*)  Src, Len);
	if ( sizeof(CHAR) == 2 ) return (CHAR*)CopyToBuffer16( (const char16*)Src, Len);
	if ( sizeof(CHAR) == 4 ) return (CHAR*)CopyToBuffer32( (const char32*)Src, Len);
}
#ifndef NO_CPP11_TEMPLATES
	template < typename CHAR, size_t ArraySize > FORCEINLINE CHAR* CopyToBuffer( CHAR (&Src)[ArraySize])
	{
		return CopyToBuffer<CHAR>( Src, ArraySize-1);
	}
#endif

//*******************************************************************
// STRING BUFFERS

extern "C"
{
	CACUS_API bool CStringBufferInit( size_t BufferSize);
	CACUS_API bool CStringBufferDeinit();
	CACUS_API uint8* CStringBuffer( size_t BufferSize); //Alignment is always platform INT, max size is 256kb
};

template<typename CHAR> CHAR* CharBuffer( uint32 CharCount)
{
	static_assert( CheckCharType<CHAR>() ,"CharBuffer: Invalid character type");
	return (CHAR*)CStringBuffer( sizeof(CHAR)*CharCount);
}



//*******************************************************************
// UTF-8

// Notes:
// The UTF8 encoder will return <0> in case of success and <char_index> in case of failure
// This is very useful for encoding a large string in parts over a small buffer

extern "C"
{
	CACUS_API size_t UTF8_EncodedLen8 ( const char*   Src);
	CACUS_API size_t UTF8_EncodedLen16( const char16* Src);
	CACUS_API size_t UTF8_EncodedLen32( const char32* Src);

	CACUS_API int UTF8_Decode8 ( char*   Dest, size_t DestSize, const char* Src);
	CACUS_API int UTF8_Decode16( char16* Dest, size_t DestSize, const char* Src);
	CACUS_API int UTF8_Decode32( char32* Dest, size_t DestSize, const char* Src);

	CACUS_API size_t UTF8_Encode8 ( char* Dest, size_t DestSize, const char*   Src);
	CACUS_API size_t UTF8_Encode16( char* Dest, size_t DestSize, const char16* Src);
	CACUS_API size_t UTF8_Encode32( char* Dest, size_t DestSize, const char32* Src);
}

struct utf8
{
	// Returns the required buffer size to hold the encoded version of a string
	template< typename CHAR > static FORCEINLINE size_t EncodedLen( const CHAR* Str)
	{
		if      ( sizeof(CHAR) == 1 ) return UTF8_EncodedLen8 ( (const char*)  Str);
		else if ( sizeof(CHAR) == 2 ) return UTF8_EncodedLen16( (const char16*)Str);
		else if ( sizeof(CHAR) == 4 ) return UTF8_EncodedLen32( (const char32*)Str);
	}

	template< typename CHAR > static FORCEINLINE int Decode( CHAR* Dest, size_t DestSize, const char* Src)
	{
		if      ( sizeof(CHAR) == 1 ) return UTF8_Decode8 ( (char*)  Dest, DestSize, Src);
		else if ( sizeof(CHAR) == 2 ) return UTF8_Decode16( (char16*)Dest, DestSize, Src);
		else if ( sizeof(CHAR) == 4 ) return UTF8_Decode32( (char32*)Dest, DestSize, Src);
	}

	template< typename CHAR > static FORCEINLINE size_t Encode( char* Dest, size_t DestSize, const CHAR* Src)
	{
		if      ( sizeof(CHAR) == 1 ) return UTF8_Encode8 ( Dest, DestSize, (const char*)  Src);
		else if ( sizeof(CHAR) == 2 ) return UTF8_Encode16( Dest, DestSize, (const char16*)Src);
		else if ( sizeof(CHAR) == 4 ) return UTF8_Encode32( Dest, DestSize, (const char32*)Src);
	}

#ifndef NO_CPP11_TEMPLATES
	template< typename CHAR, size_t DestSize > static FORCEINLINE int Decode( CHAR (&Dest)[DestSize], const char* Src)
	{	return Decode<CHAR>( Dest, DestSize, Src);	}
	template< typename CHAR, size_t DestSize > static FORCEINLINE int Encode( char (&Dest)[DestSize], const CHAR* Src)
	{	return Encode<CHAR>( Dest, DestSize, Src);	}
#endif
};


//*******************************************************************
// STREAM POSITION MANIPULATORS

extern "C"
{
	CACUS_API bool AdvanceTo8 ( const char*&   Pos, const char*   CharList);
	CACUS_API bool AdvanceTo16( const char16*& Pos, const char16* CharList);
	CACUS_API bool AdvanceTo32( const char32*& Pos, const char32* CharList);

	CACUS_API bool AdvanceThrough8 ( const char*&   Pos, const char*   CharList);
	CACUS_API bool AdvanceThrough16( const char16*& Pos, const char16* CharList);
	CACUS_API bool AdvanceThrough32( const char32*& Pos, const char32* CharList);

	CACUS_API char*   NextParameter8 ( const char*&   Pos, const char*   Delimiter); 
	CACUS_API char16* NextParameter16( const char16*& Pos, const char16* Delimiter); 
	CACUS_API char32* NextParameter32( const char32*& Pos, const char32* Delimiter);

	CACUS_API char*   ExtractLine8 ( const char*&   Pos); //Specialized newline 'NextParameter'
	CACUS_API char16* ExtractLine16( const char16*& Pos);
	CACUS_API char32* ExtractLine32( const char32*& Pos);
}

//Advances until one in CharList is found (may not advance)
template<typename CHAR> FORCEINLINE bool AdvanceTo( const CHAR*& Pos, const CHAR* CharList)
{
	if ( sizeof(CHAR) == 1 ) return AdvanceTo8 ( (const char*&)  Pos, (const char*)  CharList);
	if ( sizeof(CHAR) == 2 ) return AdvanceTo16( (const char16*&)Pos, (const char16*)CharList);
	if ( sizeof(CHAR) == 4 ) return AdvanceTo32( (const char32*&)Pos, (const char32*)CharList);
}

//Skips whatever is in CharList
template<typename CHAR> FORCEINLINE bool AdvanceThrough( const CHAR*& Pos, const CHAR* CharList)
{
	if ( sizeof(CHAR) == 1 ) return AdvanceThrough8 ( (const char*&)  Pos, (const char*)  CharList);
	if ( sizeof(CHAR) == 2 ) return AdvanceThrough16( (const char16*&)Pos, (const char16*)CharList);
	if ( sizeof(CHAR) == 4 ) return AdvanceThrough32( (const char32*&)Pos, (const char32*)CharList);
}

//Simple parameter parser/advancer (result buffer is editable)
template<typename CHAR> FORCEINLINE CHAR* NextParameter( const CHAR*& Pos, const CHAR* Delimiter)
{
	if ( sizeof(CHAR) == 1 ) return (CHAR*)NextParameter8 ( (const char*&)  Pos, (const char*)  Delimiter);
	if ( sizeof(CHAR) == 2 ) return (CHAR*)NextParameter16( (const char16*&)Pos, (const char16*)Delimiter);
	if ( sizeof(CHAR) == 4 ) return (CHAR*)NextParameter32( (const char32*&)Pos, (const char32*)Delimiter);
}

//Specialized newline 'NextParameter'
template<typename CHAR> FORCEINLINE CHAR* ExtractLine( const CHAR*& Pos)
{
	if ( sizeof(CHAR) == 1 ) return (CHAR*)ExtractLine8 ( (const char*&)  Pos);
	if ( sizeof(CHAR) == 2 ) return (CHAR*)ExtractLine16( (const char16*&)Pos);
	if ( sizeof(CHAR) == 4 ) return (CHAR*)ExtractLine32( (const char32*&)Pos);
}


//*******************************************************************
// TEMPLATED FIXERS AND EXPANDERS

// AdvanceTo - single char
template<typename CHAR> inline bool AdvanceTo( const CHAR*& Pos, CHAR Char)
{
	CHAR CharList[2] = { Char, '\0' };
	return AdvanceTo( Pos, CharList);
}

// ParseToken - modify stream pointer + allow non-const char buffers
template<typename CHAR> inline CHAR* ExtractToken( const CHAR*& Text)
{
	size_t Offset = 0;
	CHAR* Result = ParseToken( Text, &Offset);
	Text += Offset;
	return Result;
}
template<typename CHAR> inline CHAR* ExtractToken( CHAR*& Text)
{
	const CHAR* Text2 = Text;
	CHAR* Result = ExtractToken( Text2);
	Text = (CHAR*)Text2;
	return Result;
}

#endif
