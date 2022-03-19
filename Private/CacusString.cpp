
#include <cstdarg>
#include <wchar.h>

#include "CacusLibPrivate.h"

#include "CacusString.h"
#include "CacusMem.h"
#include "DebugCallback.h"

//Compiler functions
static constexpr uint16 _cf_is_upper( uint8 i)   { return (i>='A'&&i<='Z') ? CHTYPE_Upper : 0; }
static constexpr uint16 _cf_is_lower( uint8 i)   { return (i>='a'&&i<='z') ? CHTYPE_Lower : 0; }
static constexpr uint16 _cf_is_digit( uint8 i)   { return (i>='0'&&i<='9') ? CHTYPE_Digit : 0; }
static constexpr uint16 _cf_is_opera( uint8 i)   { return (i=='!'||i=='#'||i=='$'||i=='%'||i=='&'||i=='*'||i=='+'||i=='-'||i=='.'||i==':'||i==';'||i=='<'||i=='='||i=='>'||i=='@'||i=='|'||i=='~') ? CHTYPE_TokOp : 0; }
static constexpr uint16 _cf_is_cntrl( uint8 i)   { return (i<=0x1F||i==0x7F) ? CHTYPE_Cntrl : 0; }
static constexpr uint16 _cf_is_toksp( uint8 i)   { return (i==' '||i=='\r'||i=='\n'||i=='\v'||i=='\t') ? CHTYPE_TokSp : 0; }
static constexpr uint16 _cf_is_toksc( uint8 i)   { return (i=='['||i==']'||i=='{'||i=='}'||i=='('||i==')') ? CHTYPE_TokSc : 0; }

static constexpr FORCEINLINE uint16 _cf(uint8 i)
{
	return _cf_is_upper(i) | _cf_is_lower(i) | _cf_is_digit(i) | _cf_is_cntrl(i) | _cf_is_opera(i) | _cf_is_toksp(i) | _cf_is_toksc(i);
}

static const uint16 CharFlags[128] =
{
	_cf(0x00), _cf(0x01), _cf(0x02), _cf(0x03), _cf(0x04), _cf(0x05), _cf(0x06), _cf(0x07),
	_cf(0x08), _cf(0x09), _cf(0x0A), _cf(0x0B), _cf(0x0C), _cf(0x0D), _cf(0x0E), _cf(0x0F),
	_cf(0x10), _cf(0x11), _cf(0x12), _cf(0x13), _cf(0x14), _cf(0x15), _cf(0x16), _cf(0x17),
	_cf(0x18), _cf(0x19), _cf(0x1A), _cf(0x1B), _cf(0x1C), _cf(0x1D), _cf(0x1E), _cf(0x1F),
	_cf(0x20), _cf(0x21), _cf(0x22), _cf(0x23), _cf(0x24), _cf(0x25), _cf(0x26), _cf(0x27),
	_cf(0x28), _cf(0x29), _cf(0x2A), _cf(0x2B), _cf(0x2C), _cf(0x2D), _cf(0x2E), _cf(0x2F),
	_cf(0x30), _cf(0x31), _cf(0x32), _cf(0x33), _cf(0x34), _cf(0x35), _cf(0x36), _cf(0x37),
	_cf(0x38), _cf(0x39), _cf(0x3A), _cf(0x3B), _cf(0x3C), _cf(0x3D), _cf(0x3E), _cf(0x3F),
	_cf(0x40), _cf(0x41), _cf(0x42), _cf(0x43), _cf(0x44), _cf(0x45), _cf(0x46), _cf(0x47),
	_cf(0x48), _cf(0x49), _cf(0x4A), _cf(0x4B), _cf(0x4C), _cf(0x4D), _cf(0x4E), _cf(0x4F),
	_cf(0x50), _cf(0x51), _cf(0x52), _cf(0x53), _cf(0x54), _cf(0x55), _cf(0x56), _cf(0x57),
	_cf(0x58), _cf(0x59), _cf(0x5A), _cf(0x5B), _cf(0x5C), _cf(0x5D), _cf(0x5E), _cf(0x5F),
	_cf(0x60), _cf(0x61), _cf(0x62), _cf(0x63), _cf(0x64), _cf(0x65), _cf(0x66), _cf(0x67),
	_cf(0x68), _cf(0x69), _cf(0x6A), _cf(0x6B), _cf(0x6C), _cf(0x6D), _cf(0x6E), _cf(0x6F),
	_cf(0x70), _cf(0x71), _cf(0x72), _cf(0x73), _cf(0x74), _cf(0x75), _cf(0x76), _cf(0x77),
	_cf(0x78), _cf(0x79), _cf(0x7A), _cf(0x7B), _cf(0x7C), _cf(0x7D), _cf(0x7E), _cf(0x7F)
};

/* ==============================================
	CHAR TYPES CHECKER
*/

uint32 CGetCharType(uint32 ch)      { return (ch < 128) ? CharFlags[ch] : 0; }
bool CChrIsDigit(uint32 ch)           { return (CGetCharType(ch) & CHTYPE_Digit) != 0;}


bool CChrIsUpper( uint32 ch)
{
	if ( ch < 128 )
		return (CharFlags[ch] & CHTYPE_Upper) != 0;
	return (ch >= 0xC0 /*À*/ && ch <= 0XDE /*Þ*/ && ch != 0xD7 /*×*/ );
}

bool CChrIsLower( uint32 ch)
{
	if ( ch < 128 )
		return (CharFlags[ch] & CHTYPE_Lower) != 0;
	return (ch >= 0xE0 /*à*/ && ch <= 0XFE /*þ*/ && ch != 0xF7 /*÷*/ );
}


/* ==============================================
	STRING BUFFER HANDLING (REDUCE TEMPLATE CODE IN IMPLEMENTATIONS)
*/


struct __controller
{
	CCircularBuffer* Inner;
	__controller() : Inner(nullptr) {};
	~__controller() { if (Inner) CircularFree(Inner); Inner=nullptr; }
};
#if WINDOWS_XP_SUPPORT
static __controller StringBuffer; //This autodeletes the buffer on 
#else
static thread_local __controller StringBuffer; //This autodeletes the buffer on 
#endif

bool CStringBufferInit( size_t BufferSize)
{
	if ( StringBuffer.Inner )
		return false;
	StringBuffer.Inner = CircularAllocate( BufferSize);
	return true;
}

bool CStringBufferDeinit()
{
	if ( !StringBuffer.Inner )
		return false;
	CircularFree( StringBuffer.Inner);
	StringBuffer.Inner = nullptr;
	return true;
}

uint8* CStringBuffer( size_t BufferSize)
{
	if ( !StringBuffer.Inner )
		CStringBufferInit( 256*1024);
	uint8* Result = StringBuffer.Inner->Request<EALIGN_PLATFORM_PTR,false>(BufferSize);
	if ( !Result )
	{
		static char Buf[256];
		sprintf( Buf, "Requested more data than available from String Buffer [%i/%i]", (int)BufferSize, (int)StringBuffer.Inner->Size() );
		DebugCallback( Buf, CACUS_CALLBACK_STRING | CACUS_CALLBACK_EXCEPTION);
	}
	return Result;
}


/* ==============================================
   STRING COPY LITERALS, SAFE VERSIONS INCLUDED
*/

#define EINVAL 22
#define ERANGE 34
template <typename CHARSRC,typename CHARDEST> FORCEINLINE int templ_strcpy_s( CHARDEST* Dest, size_t DestChars, const CHARSRC* Src)
{
	uint32 i;
	if( !Dest || !DestChars )
		return EINVAL;
	if ( !Src )
	{
		Dest[0] = '\0';
		return EINVAL;
	}

	for ( i=0 ; i<DestChars ; i++ )
		if ( (Dest[i] = (CHARDEST)Src[i]) == '\0' )
			return 0;
	Dest[DestChars-1] = '\0';
	return ERANGE;
}
int CStrcpy8_s ( char*   Dest, size_t DestChars, const char*   Src) { return templ_strcpy_s( Dest, DestChars, Src); }
int CStrcpy16_s( char16* Dest, size_t DestChars, const char16* Src) { return templ_strcpy_s( Dest, DestChars, Src); }
int CStrcpy32_s( char32* Dest, size_t DestChars, const char32* Src) { return templ_strcpy_s( Dest, DestChars, Src); }

int CStrcpy_g_s( void* Dest, size_t DestBytes, size_t DestChars, const void* Src, size_t SrcBytes)
{
	switch( DestBytes)
	{
	case 1:
		switch (SrcBytes) {
		case 1:	return CStrcpy8_s( (char*)Dest, DestChars, (const char*)Src);
		case 2: return templ_strcpy_s( (char*)Dest, DestChars, (const char16*)Src);
		case 4: return templ_strcpy_s( (char*)Dest, DestChars, (const char32*)Src);
		} break;
	case 2:
		switch (SrcBytes) {
		case 1:	return templ_strcpy_s( (char16*)Dest, DestChars, (const char*)Src);
		case 2: return CStrcpy16_s( (char16*)Dest, DestChars, (const char16*)Src);
		case 4: return templ_strcpy_s( (char16*)Dest, DestChars, (const char32*)Src);
		} break;
	case 4:
		switch (SrcBytes) {
		case 1:	return templ_strcpy_s( (char32*)Dest, DestChars, (const char*)Src);
		case 2: return templ_strcpy_s( (char32*)Dest, DestChars, (const char16*)Src);
		case 4: return CStrcpy32_s( (char32*)Dest, DestChars, (const char32*)Src);
		} break;
	}
	return EINVAL;
}

/* ==============================================
	STRING CONCATENATION
*/
template<typename CHAR> inline CHAR* templ_strcat_s( CHAR* Dest, size_t DestSize, const CHAR* Src)
{
	if ( DestSize )
	{
		CHAR* top = Dest+(DestSize-1); //Last usable in buffer
		CHAR* s = Dest;
		while (*s) s++;
		while ( s<top && *Src )
			*s++ = *Src++;
		*s = '\0';
	}
	return Dest;
}
char*   CStrcat8_s ( char*   Dest, size_t DestSize, const char*   Src) { return templ_strcat_s( Dest, DestSize, Src); }
char16* CStrcat16_s( char16* Dest, size_t DestSize, const char16* Src) { return templ_strcat_s( Dest, DestSize, Src); }
char32* CStrcat32_s( char32* Dest, size_t DestSize, const char32* Src) { return templ_strcat_s( Dest, DestSize, Src); }

/* ==============================================
	STRING LENGTH
*/
template<typename CHAR> inline size_t templ_strlen( const CHAR* Str)
{
	if( !Str )
		return 0;
	const CHAR* s;
	for( s=Str ; *s ; ++s );
	return s - Str;
}
size_t CStrlen8 ( const char*  Str)  { return templ_strlen(Str); }
size_t CStrlen16( const char16* Str) { return templ_strlen(Str); }
size_t CStrlen32( const char32* Str) { return templ_strlen(Str); }

/* ==============================================
	CHAR FIND
*/
template<typename CHAR> inline CHAR* templ_strchr( const CHAR* Str, int32 Find)
{
	CHAR c = (CHAR)Find; //Convert

	for ( ; *Str ; Str++ )
		if ( *Str == c )
			return (CHAR*)Str;
	if ( c == '\0' ) //Zero-terminator needs this
		return (CHAR*)Str;
	return nullptr;
}
char*   CStrchr8 ( const char*   Str, int32 Find) { return templ_strchr(Str,Find); }
char16* CStrchr16( const char16* Str, int32 Find) { return templ_strchr(Str,Find); }
char32* CStrchr32( const char32* Str, int32 Find) { return templ_strchr(Str,Find); }


/* ==============================================
	STRING FIND - scans first character, then whole string
*/
template<typename CHAR> inline CHAR* templ_strstr( const CHAR* Str, const CHAR* Find)
{
	CHAR c, sc;

	if ( (c=*Find++) != '\0' )
	{
		const size_t len = CStrlen(Find);
		do {
			do {
				if ( (sc=*Str++) == '\0' )
					return 0;
			} while (sc != c);
		} while (CStrncmp(Str, Find, len) != 0);
		Str--;
	}
	return (CHAR*)((size_t)Str);
}
char*   CStrstr8 ( const char*   Str, const char*   Find) { return templ_strstr(Str,Find); }
char16* CStrstr16( const char16* Str, const char16* Find) { return templ_strstr(Str,Find); }
char32* CStrstr32( const char32* Str, const char32* Find) { return templ_strstr(Str,Find); }


/* ==============================================
	STRING COMPARE 
*/
template<typename CHAR> inline int templ_strcmp( const CHAR* S1, const CHAR* S2)
{
	for ( ; *S1==*S2 ; S1++, S2++)
		if ( *S1 == '\0' )
			return 0;
	return *S1 - *S2;
}
int CStrcmp8 ( const char*   S1, const char*   S2) { return templ_strcmp(S1,S2); }
int CStrcmp16( const char16* S1, const char16* S2) { return templ_strcmp(S1,S2); }
int CStrcmp32( const char32* S1, const char32* S2) { return templ_strcmp(S1,S2); }


/* ==============================================
	STRING COMPARE N
*/
template<typename CHAR> inline int templ_strncmp( const CHAR* S1, const CHAR* S2, size_t len)
{
	while ( len-- )
	{
		const CHAR c1 = *S1;
		const CHAR c2 = *S2;
		if( c1 != c2 )
			return (int)c1 - (int)c2;
		if( c1 == '\0' )
			break;
		S1++;
		S2++;
	}
	return 0;
}
int CStrncmp8 ( const char*   S1, const char*   S2, size_t len) { return templ_strncmp(S1,S2,len); }
int CStrncmp16( const char16* S1, const char16* S2, size_t len) { return templ_strncmp(S1,S2,len); }
int CStrncmp32( const char32* S1, const char32* S2, size_t len) { return templ_strncmp(S1,S2,len); }


/* ==============================================
	STRING COMPARE I
*/
template<typename CHAR> inline int templ_stricmp( const CHAR* S1, const CHAR* S2)
{
	for ( ; CChrToUpper(*S1)==CChrToUpper(*S2) ; S1++, S2++)
		if ( *S1 == '\0' )
			return 0;
	return *S1 - *S2;
}
int CStricmp8 ( const char*   S1, const char*   S2) { return templ_stricmp(S1,S2); }
int CStricmp16( const char16* S1, const char16* S2) { return templ_stricmp(S1,S2); }
int CStricmp32( const char32* S1, const char32* S2) { return templ_stricmp(S1,S2); }


/* ==============================================
	STRING COMPARE N I
*/
template<typename CHAR> inline int templ_strnicmp( const CHAR* S1, const CHAR* S2, size_t len)
{
	while ( len-- )
	{
		const CHAR c1 = CChrToUpper(*S1);
		const CHAR c2 = CChrToUpper(*S2);
		if( c1 != c2 )
			return (int)c1 - (int)c2;
		if( c1 == '\0' )
			break;
		S1++;
		S2++;
	}
	return 0;
}
int CStrnicmp8 ( const char*   S1, const char*   S2, size_t len) { return templ_strnicmp(S1,S2,len); }
int CStrnicmp16( const char16* S1, const char16* S2, size_t len) { return templ_strnicmp(S1,S2,len); }
int CStrnicmp32( const char32* S1, const char32* S2, size_t len) { return templ_strnicmp(S1,S2,len); }


/* ==============================================
	STRING PRINTF INTO CIRCULAR BUFFER
*/

char* VARARGS CSprintf( const char* fmt, ...)
{
	va_list args, args2;

	va_start(args, fmt);
	va_copy(args2,args);
#ifdef _WINDOWS
	int Length = _vscprintf( fmt, args2); //vsnprintf was implemented in UCRT!! (2015)
#else
	int Length = vsnprintf( nullptr, 0, fmt, args2); 
#endif
	va_end(args2);

	char* Result = (char*)U"";
	if ( Length > 0 )
	{
		char* Buffer = CharBuffer<char>(Length+1);
		if ( Buffer )
		{
			Result = Buffer;
			vsprintf( Result, fmt, args);
		}
	}
	va_end(args);
	return Result;
}

wchar_t* VARARGS CWSprintf( const wchar_t* fmt, ...)
{
	va_list args, args2;

	va_start(args, fmt);
	va_copy(args2,args);
#ifdef _WINDOWS
	int Length = _vscwprintf( fmt, args2); //vsnprintf was implemented in UCRT!! (2015)
#else
	int Length = vswprintf( nullptr, 0, fmt, args2); 
#endif
	va_end(args2);

	wchar_t* Result = (wchar_t*)U"";
	if ( Length > 0 )
	{
		wchar_t* Buffer = CharBuffer<wchar_t>(Length+1);
		if ( Buffer )
		{
			Result = Buffer;
			vswprintf( Result, Length+1, fmt, args);
		}
	}
	va_end(args);
	return Result;
}

/* ==============================================
	STRING UTILS
*/

//====== EVALUATES IF STRING IS A NUMBER
template<typename CHAR> inline bool templ_is_numeric( const CHAR* Str) noexcept
{
	const CHAR* Start = Str;
	for ( ; *Str ; Str++ )
		if ( !CChrIsDigit(*Str) )
			return false;
	return Start != Str;
}
bool IsNumeric8 ( const char*   Str) { return templ_is_numeric(Str); }
bool IsNumeric16( const char16* Str) { return templ_is_numeric(Str); }
bool IsNumeric32( const char32* Str) { return templ_is_numeric(Str); }

//====== EVALUATES IF STRING IS A NUMBER OR LETTER
template<typename CHAR> inline bool templ_is_alphanumeric( const CHAR* Str) noexcept
{
	const CHAR* Start = Str;
	for ( ; *Str ; Str++ )
		if ( !(CGetCharType(*Str) & CHTYPE_AlNum) )
			return false;
	return Start != Str;
}
bool IsAlphaNumeric8 ( const char*   Str) { return templ_is_alphanumeric(Str); }
bool IsAlphaNumeric16( const char16* Str) { return templ_is_alphanumeric(Str); }
bool IsAlphaNumeric32( const char32* Str) { return templ_is_alphanumeric(Str); }


//====== OPERATING SYSTEM PATH CONVERSION, ex: Turns "dir\file" into "dir/file"
#if _WINDOWS
#else
	#define PATH_BAD '\\'
	#define PATH_GOOD '/'

template<typename CHAR> inline const CHAR* templ_OSpath( const CHAR* Path)
{
	bool bNeedsFix = false;
	uint32 i;
	for ( i=0 ; (Path[i] != '\0') ; i++ ) //Count length of text and determine if needs fix
		bNeedsFix = bNeedsFix || Path[i]==PATH_BAD;

	if ( bNeedsFix )
	{
		const uint32 PathSize = i + 1; //Size = Length + 1
		CHAR* FixedPath = CharBuffer<CHAR>(PathSize); //Request a buffer just large enough to contain the fixed path
		for ( i=0 ; i<PathSize ; i++ )
			FixedPath[i] = (Path[i]==PATH_BAD) ? PATH_GOOD : Path[i]; //Copy and fix onto buffer
		return FixedPath;
	}
	return Path;
}
const char* OSpath( const char* Path) { return templ_OSpath( Path); }
#endif

//====== COPIES INPUT INTO AN EDITABLE BUFFER
template < typename CHAR > CHAR* templ_copy_to_buffer( const CHAR* Src, size_t Len)
{
	if ( !Len )
		return (CHAR*)U""; //Done on purpose to return pointer to 32-bit zero element
	for ( size_t i=0 ; i<Len ; i++ )
		if ( Src[i] == '\0' )
		{
			Len = i;
			break;
		}
	CHAR* Buffer = CharBuffer<CHAR>(Len+1);
	if ( Buffer )
	{
		CMemcpy( Buffer, Src, Len*sizeof(CHAR) );
		Buffer[Len] = 0;
	}
	return Buffer;
}
char*   CopyToBuffer8 ( const char*   Src, const size_t Len)  { return templ_copy_to_buffer( Src, Len); }
char16* CopyToBuffer16( const char16* Src, const size_t Len)  { return templ_copy_to_buffer( Src, Len); }
char32* CopyToBuffer32( const char32* Src, const size_t Len)  { return templ_copy_to_buffer( Src, Len); }


//====== TOKEN PARSER, SINGLE WORD OR QUOTED STRING
static bool _parse_token_float_number( const char*& Stream)
{
	const char* Cur = Stream;
	if ( *Cur == '-' || *Cur == '+' )
		Cur++;

	if ( CChrIsDigit(*Cur) ) {} //Ok ( "n" , "-n" )
	else if ( (*Cur == '.') && CChrIsDigit(*(Cur+1)) ) {} //Ok ( ".n" , "-.n" )
	else	return false; //Incorrect format

	for ( ; CChrIsDigit(*Cur) ; Cur++ );
	if ( *Cur == '.' ) //Dot
	{
		for ( Cur++ ; CChrIsDigit(*Cur) ; Cur++ );
	}
	if ( *Cur == 'e' || *Cur == 'E' ) //Exponent
	{
		for ( Cur++ ; CChrIsDigit(*Cur) ; Cur++ );
	}
	if ( *Cur == 'f' || *Cur == 'F' ) //Float
		Cur++;
	Stream = Cur; //Offset stream
	return true;
}

char* ParseToken( const char* Text, size_t* ModifyPos)
{
	if ( !Text )
		return nullptr;

	const char* TokenStart;
	for ( TokenStart=Text ; CGetCharType(*TokenStart)&CHTYPE_TokSp ; TokenStart++ ); //Skip thru token splitters

	const char* TokenEnd = TokenStart;
	const char* TextEnd = nullptr;
	if ( *TokenStart == '\0' )
		return nullptr;
	else if ( *TokenStart == '\x22' || *TokenStart == '\x27' ) //Quoted string
	{
		char Delimiter = *TokenStart++;
		TokenEnd++;
		AdvanceTo( TokenEnd, Delimiter);
		TextEnd = TokenEnd + (*TokenEnd==Delimiter); //Skip ending delimiter too
	}
	else if ( CGetCharType(*TokenStart) & CHTYPE_TokOp ) //Special token
	{
		if ( !_parse_token_float_number( TokenEnd ) ) //If not float, parse as operator (skip first char)
			for ( TokenEnd++ ; CGetCharType(*TokenEnd) & CHTYPE_TokOp ; TokenEnd++ );
	}
	else if ( CGetCharType(*TokenStart) & CHTYPE_TokSc ) //Single character
		TokenEnd++;
	else
	{
		if ( !_parse_token_float_number( TokenEnd ) )
			for ( ; (CGetCharType(*TokenEnd) & CHTYPE_AlNum) || *TokenEnd=='_' ; TokenEnd++ );
	}


	if ( !TextEnd )
		TextEnd = TokenEnd + (TokenEnd == TokenStart); //Prevent infinite loop

	while ( CGetCharType(*TextEnd) & CHTYPE_TokSp  )
		TextEnd++;

	//Tight buffer
	size_t TokenLength = ((size_t)TokenEnd - (size_t)TokenStart) / sizeof(char);
	if ( ModifyPos )
		*ModifyPos += ((size_t)TextEnd - (size_t)Text) / sizeof(char); //Buffer skipping
	char* Result = CharBuffer<char>(TokenLength+1);
	if ( Result )
	{
		if ( TokenLength )
			CMemcpy( Result, TokenStart, TokenLength*sizeof(char) );
		Result[TokenLength] = 0;
	}
//	printf("Token: (%i) %s \n", TokenLength, Result);
	return Result;
}


/* ==============================================
STRING MANIPULATORS
*/

//Move Pos to any of the characters in the list
template<typename CHAR> inline bool templ_advance_to( const CHAR*& Pos, const CHAR* CharList)
{
	while ( *Pos )
	{
		const CHAR C = *Pos;
		for ( uint32 i=0 ; CharList[i] ; i++ )
			if ( C == CharList[i] )
				return true;
		Pos++;
	}
	return false;
}
bool AdvanceTo8 ( const char*& Pos, const char* CharList)      { return templ_advance_to(Pos,CharList); }
bool AdvanceTo16( const char16*& Pos, const char16* CharList)  { return templ_advance_to(Pos,CharList); }
bool AdvanceTo32( const char32*& Pos, const char32* CharList)  { return templ_advance_to(Pos,CharList); }

//Skip all characters in the list
template<typename CHAR> inline bool templ_advance_through( const CHAR*& Pos, const CHAR* CharList)
{
	while ( const CHAR C=*Pos )
	{
		bool bSkip = false;
		for ( uint32 i=0 ; CharList[i] ; i++ )
			bSkip = bSkip | (C == CharList[i]);
		if ( !bSkip )
			return true;
		Pos++;
	}
	return false;
}
bool AdvanceThrough8 ( const char*& Pos, const char* CharList)      { return templ_advance_through(Pos,CharList); }
bool AdvanceThrough16( const char16*& Pos, const char16* CharList)  { return templ_advance_through(Pos,CharList); }
bool AdvanceThrough32( const char32*& Pos, const char32* CharList)  { return templ_advance_through(Pos,CharList); }

//Grab a parameter and advance
template<typename CHAR> inline CHAR* templ_next_parameter( const CHAR*& Pos, const CHAR* Delimiter)
{
	const CHAR* Found = CStrstr(Pos,Delimiter);
	size_t len = Found 
				? (((size_t)Found - (size_t)Pos) / sizeof(CHAR))
				: CStrlen(Pos);
	if ( len )
	{
		CHAR* Buffer = CharBuffer<CHAR>( len + 1);
		if ( Buffer )
		{
			CMemcpy( Buffer, Pos, len*sizeof(CHAR));
			Buffer[len] = 0;
			Pos += len;
			if ( Found )
				Pos += CStrlen(Delimiter);
			return Buffer;
		}
	}
	return nullptr;
}
char*   NextParameter8 ( const char*& Pos, const char* Delimiter)     { return templ_next_parameter(Pos,Delimiter); }
char16* NextParameter16( const char16*& Pos, const char16* Delimiter) { return templ_next_parameter(Pos,Delimiter); }
char32* NextParameter32( const char32*& Pos, const char32* Delimiter) { return templ_next_parameter(Pos,Delimiter); }

//Extracts a line from a stream (follows Unix/MAC/Windows specs, sort of)
template<typename CHAR> inline CHAR* templ_extract_line( const CHAR*& Pos)
{
	if ( !*Pos )
		return nullptr;
	const CHAR* StartPos = Pos;
	const CHAR* CurPos = Pos;
	while ( *CurPos && (*CurPos != '\r') && (*CurPos != '\n') )
		CurPos++;
	size_t Len = ((size_t)CurPos - (size_t)StartPos) / sizeof(CHAR);
	CHAR* Buf = CharBuffer<CHAR>( Len + 1);
	CMemcpy( Buf, StartPos, (size_t)CurPos-(size_t)StartPos);
	Buf[Len] = 0;
	if ( *CurPos == '\r' ) CurPos++;
	if ( *CurPos == '\n' ) CurPos++;
	Pos = CurPos;
	return Buf;
}
char*   ExtractLine8 (const char  *& Pos) { return templ_extract_line(Pos); }
char16* ExtractLine16(const char16*& Pos) { return templ_extract_line(Pos); }
char32* ExtractLine32(const char32*& Pos) { return templ_extract_line(Pos); }
