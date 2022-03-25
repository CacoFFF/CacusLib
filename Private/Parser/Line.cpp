/*=============================================================================
	Parser/Line.cpp:
	Simple text line parser implementation for Cacus library.
	Author: Fernando Velázquez
=============================================================================*/

#include "../CacusLibPrivate.h"

#include "CacusString.h"
#include "CacusTemplate.h"

#include "CacusMem.h"
#include "Parser/Line.h"

static const char* DefaultLineBreaks8[] =
{
	"\r\n",
	"\n",
	"\r",
};

static const char* DefaultLineArray[] =
{
	nullptr
};



//========= templ_strcmp_token - begin ==========//
//
// Checks if the Token matches the start of the String
//
template<typename CHAR> inline bool templ_strcmp_token( const CHAR* String, const CHAR* Token)
{
	while ( *Token != '\0' )
	{
		if( *String != *Token )
			return false;
		String++;
		Token++;
	}
	return true;
}
//========= templ_strcmp_token - end ==========//




//========= CParserFastLine8 constructor - begin ==========//
//
CParserFastLine8::CParserFastLine8()
	: LineCount(0)
	, LineArray(DefaultLineArray)
	, Data(nullptr)
	, LineBreakCount(ArrayCount(DefaultLineBreaks8))
	, LineBreakArray(DefaultLineBreaks8)
{
}
//========= CParserFastLine8 constructor - end ==========//


//========= CParserFastLine8 destructor - begin ==========//
// 
// Ensure memory deallocation.
//
CParserFastLine8::~CParserFastLine8()
{
	if ( Data )
	{
		CFree(Data);
		Data = nullptr;
	}
}
//========= CParserFastLine8 destructor - end ==========//


//========= CParserFastLine8::SetLineBreaks - begin ==========//
// 
// Replaces tokens used to separate lines.
//
void CParserFastLine8::SetLineBreaks( const char** NewLineBreakArray, size_t NewLineBreakCount)
{
	LineBreakArray = NewLineBreakArray;
	LineBreakCount = NewLineBreakCount;
}
//========= CParserFastLine8::SetLineBreaks - end ==========//


//========= CParserFastLine8::Parse - begin ==========//
// 
// Parses given Text into a line array of char streams.
// The parser uses a single memory allocation to store all of the resulting data.
// 
// <bool> return value - Parsing finished without errors.
//
bool CParserFastLine8::Parse( const char* Text)
{
	typedef char CHAR;
	struct LineInfo
	{
		LineInfo*   Next;
		const CHAR* LineStart;
		const CHAR* LineEnd;
	};

	// Set defaults
	LineCount = 0;
	LineArray = DefaultLineArray;
	if ( Data )
		CFree(Data);
	Data = nullptr;

	if ( !Text || (*Text == '\0') )
		return false;

	CMemExStack MemStack;
	LineInfo* LineInfoChain = nullptr;

	const CHAR* CurLineStart = Text;

NEXT_CHAR:
	// End of text
	if ( *Text == '\0')
	{
		LineInfoChain = new(MemStack) LineInfo{LineInfoChain, CurLineStart, Text};
		goto END_OF_TEXT;
	}

	// Token
	for ( size_t i=0; i<LineBreakCount; i++)
		if ( templ_strcmp_token(Text, LineBreakArray[i]) )
		{
			LineInfoChain = new(MemStack) LineInfo{LineInfoChain, CurLineStart, Text};
			Text += CStrlen(LineBreakArray[i]);
			CurLineStart = Text;
			goto NEXT_CHAR;
		}

	Text++;
	goto NEXT_CHAR;

END_OF_TEXT:
	// Count LineInfos and calc required memory
	size_t LineInfoCount = 0;
	size_t LineMemorySize = 0;
	for ( LineInfo* Link=LineInfoChain; Link; Link=Link->Next)
	{
		LineInfoCount++;

		size_t RequiredMemory = (((size_t)Link->LineEnd) - ((size_t)Link->LineStart)) + sizeof(CHAR);
		LineMemorySize = Align(LineMemorySize + RequiredMemory, EALIGN_PLATFORM_PTR);
	}
	LineMemorySize += EALIGN_PLATFORM_PTR * (LineInfoCount+1+1); // Add space for LineArray, and safety padding

	// Prepare data chunk
	// Uses ScopedMemory for automatic deallocation upon failure
	CScopeMem NewData(LineMemorySize);

	// Temporarily fill LineInfo array in correct order (they were inverted due to Linked List)
	LineInfo** LineInfoArray = NewData.GetArray<LineInfo*>();
	size_t i = LineInfoCount;
	LineInfoArray[i] = nullptr;
	for ( LineInfo* Link=LineInfoChain; Link; Link=Link->Next)
		LineInfoArray[--i] = Link;

	// Process each array element and convert it into a char pointer
	CHAR** NewLineArray = NewData.GetArray<CHAR*>();
	CHAR* C = Align((CHAR*)AddressOffset(NewLineArray, EALIGN_PLATFORM_PTR * (LineInfoCount+1)), EALIGN_PLATFORM_PTR);
	for ( i=0; i<LineInfoCount; i++)
	{
		// Set Line
		LineInfo* L = LineInfoArray[i];
		NewLineArray[i] = C;

		// Copy Line
		// TODO: IMPLEMENT CStrncpy
		size_t CopySize = (size_t)L->LineEnd - (size_t)L->LineStart;
		if ( CopySize > 0 )
		{
			CMemcpy(C, L->LineStart, CopySize);
			C = (CHAR*)(((size_t)C) + CopySize);
		}

		// Fill remainder with zeroes (null terminator is added here)
		CHAR* NextC = AddressAlign(C+1, EALIGN_PLATFORM_PTR);
		while ( C < NextC )
			*C++ = '\0';
	}


	LineCount = LineInfoCount;
	LineArray = (const char**)NewLineArray;
	Data = NewData.Detach();
	return true;
}
//========= CParserFastLine8::Parse - end ==========//
