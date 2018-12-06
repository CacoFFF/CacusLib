/*=============================================================================
	Unicode.cpp:
	Simple unicode method implementations for Cacus library.
	Author: Fernando Velázquez

	UTF-8 - https://tools.ietf.org/html/rfc3629
	This Unicode module follows the RFC 3629 standard and immediately
	stops operations upon finding an error, no exceptions thrown.
=============================================================================*/

#include "CacusString.h"
#include "DebugCallback.h"



//========= UTF8 length - begin ==========//
//
// Gets the post UTF8 encode length of a string
//
template<typename CHAR> size_t templ_utf8_encoded_len( const CHAR* Src)
{
	size_t i=0;
	while ( int32 SC=*Src )
	{
		register uint32 C = *(uint32*)(&SC);
		if ( C < 0x80UL )
			i++;
		else if ( C < 0x800UL )
			i += 2;
		else if ( C < 0x10000UL )
			i += 3;
		else if ( C < 0x110000UL ) //Never happens with char16_t
			i += 4;
		Src++;
	}
	return i;
}
size_t UTF8_EncodedLen8 ( const char*   Src) { return templ_utf8_encoded_len( Src); }
size_t UTF8_EncodedLen16( const char16* Src) { return templ_utf8_encoded_len( Src); }
size_t UTF8_EncodedLen32( const char32* Src) { return templ_utf8_encoded_len( Src); }
//========= UTF8 length - end ==========//



//========= UTF8 Decoder - begin ==========//
//
// Dest     - Pointer to destination buffer
// DestSize - Size in number of chars of destination buffer
// Src      - Source text to decode
//
static bool utf8_HasConts( const char* Pos, uint32 Conts);
static uint32 utf8_GetCont( const char*& Pos);
template<typename CHAR> int templ_utf8_decode( CHAR* Dest, size_t DestSize, const char* Src)
{
	if ( !DestSize )
		return 0;

	const char* Pos = Src;
	CHAR* DestLast = &Dest[DestSize-1];
	while ( *Pos && (Dest < DestLast) )
	{
		uint32 C = *Pos++;
		// 0000 0000 - 0000 007F
		if ( (C & 0b10000000) == 0 ) //0b0xxxxxxx
		{}
		// 0000 0080 - 0000 07FF
		else if ( ((C & 0b11100000) == 0b11000000) && utf8_HasConts(Pos,1) ) //0b110xxxxx
		{
			C &= 0b00011111;
			uint32 C1 = utf8_GetCont(Pos);
			C = C1 | (C << 6);
			if ( C < 0x80 )
				break;
		}
		// 0000 0800 - 0000 FFFF
		else if ( ((C & 0b11110000) == 0b11100000) && utf8_HasConts(Pos,2) ) //0b1110xxxx
		{
			C &= 0b00001111;
			uint32 C1 = utf8_GetCont(Pos);
			uint32 C2 = utf8_GetCont(Pos);
			C = C2 | (C1 << 6) | (C << 12);
			if ( (C < 0x800) || (C >= 0xD800 && C <= 0xDFFF) )
				break;
		}
		// 0001 0000 - 0010 FFFF
		else if ( ((C & 0b11111000) == 0b11110000) && utf8_HasConts(Pos,3) ) //0b11110xxx
		{
			C &= 0b00000111;
			uint32 C1 = utf8_GetCont(Pos);
			uint32 C2 = utf8_GetCont(Pos);
			uint32 C3 = utf8_GetCont(Pos);
			C = C3 | (C2 << 6) | (C1 << 12) | (C << 18);
			if ( (C < 0x10000) || (C > 0x10FFFF) )
				break;
		}
		else
			break;

		if ( !C )	break;
		if ( (sizeof(CHAR) <= 1) && (C > MAXBYTE) )
		{
			DebugCallback( CSprintf("UTF8_Decode cannot decode %i into 1 byte char", C), CACUS_CALLBACK_STRING | CACUS_CALLBACK_EXCEPTION );
			break;
		}
		if ( (sizeof(CHAR) <= 2) && (C > MAXWORD) )
		{
			DebugCallback( CSprintf("UTF8_Decode cannot decode %i into 2 bytes char", C), CACUS_CALLBACK_STRING | CACUS_CALLBACK_EXCEPTION );
			break;
		}
		*Dest++ = (CHAR)C;
	}

	*Dest = '\0';
	return 0;
}
int UTF8_Decode8 ( char*   Dest, size_t DestSize, const char* Src) { return templ_utf8_decode( Dest, DestSize, Src); }
int UTF8_Decode16( char16* Dest, size_t DestSize, const char* Src) { return templ_utf8_decode( Dest, DestSize, Src); }
int UTF8_Decode32( char32* Dest, size_t DestSize, const char* Src) { return templ_utf8_decode( Dest, DestSize, Src); }

static bool utf8_HasConts( const char* Pos, uint32 Conts)
{
	for ( ; Conts-- ; Pos++ )
		if ( (*Pos & 0b11000000) != 0b10000000 )
			return false;
	return true;
}

static inline uint32 utf8_GetCont( const char*& Pos)
{
	if ( *Pos == 0 )
		return 0;
	return (*Pos++ & 0b00111111);
}
//========= UTF8 Decoder - end ==========//




//========= UTF8 Encoder - begin ==========//
//
// Dest     - Pointer to destination buffer
// DestSize - Size in number of chars of destination buffer
// Src      - Source text to encode
// Return   - 0 if OK, 'n' being Src[n] where it stopped in case of error.
//
template<typename CHAR> size_t templ_utf8_encode( char* Dest, size_t DestSize, const CHAR* Src)
{
	size_t SrcCount = 0;
	char* DestLast = Dest + DestSize - 1;
	while ( uint32 C=*Src )
	{
		if ( C < 0x80UL )
		{
			if ( Dest + 1 >= DestLast ) break;
			*Dest++ = (char)C;
		}
		else if ( C < 0x800UL )
		{
			if ( Dest + 2 >= DestLast ) break;
			*Dest++ = (char)(0b11000000U | (C >> 6));
			*Dest++ = 0b10000000U | (0b00111111U & C);
		}
		else if ( C < 0x10000UL )
		{
			if ( C >= 0xD800 && C <= 0xDFFF ) break;
			if ( Dest + 3 >= DestLast ) break;
			*Dest++ = (char)(0b11100000U | (C >> 12));
			*Dest++ = 0b10000000U | (0b00111111U & (C >> 6));
			*Dest++ = 0b10000000U | (0b00111111U & C);
		}
		else if ( C < 0x110000UL )
		{
			if ( Dest + 4 >= DestLast ) break;
			*Dest++ = (char)(0b11110000U | (C >> 18));
			*Dest++ = 0b10000000U | (0b00111111U & (C >> 12));
			*Dest++ = 0b10000000U | (0b00111111U & (C >> 6));
			*Dest++ = 0b10000000U | (0b00111111U & C);
		}
		else
		{
			SrcCount++;
			break;
		}
		SrcCount++;
		Src++;
	}
	*Dest = 0;
	if ( *Src )
		return SrcCount;
	return 0;
}
size_t UTF8_Encode8 ( char* Dest, size_t DestSize, const char*   Src) { return templ_utf8_encode( Dest, DestSize, Src); }
size_t UTF8_Encode16( char* Dest, size_t DestSize, const char16* Src) { return templ_utf8_encode( Dest, DestSize, Src); }
size_t UTF8_Encode32( char* Dest, size_t DestSize, const char32* Src) { return templ_utf8_encode( Dest, DestSize, Src); }
//========= UTF8 Encoder - end ==========//





