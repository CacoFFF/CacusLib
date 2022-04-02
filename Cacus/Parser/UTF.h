/*=============================================================================
	Parser/UTF.h
	Author: Fernando Velázquez

	Very simple general purpose UTF-8/UTF-16 parser.
	This is a wrapper for the UTF encoders/decoders.
=============================================================================*/

#include "../CacusString.h"

class CParserUTF
{
protected:
	CScopeMem OutputData;

public:
	size_t         Length;
	const wchar_t* Output; // Default output


public:
	CParserUTF();

	bool Parse( const void* Input);
	// TODO: Parse with known Length

protected:
	bool ParseUTF8( const char* InputData, size_t InputLength);
};



inline CParserUTF::CParserUTF()
	: Length(0)
	, Output(nullptr)
{
}

inline bool CParserUTF::Parse( const void* Input)
{
	Length = 0;
	Output = nullptr;

	if ( !Input )
		return false;

	// Find a BOM
	const char* InChars = (const char*)Input;
	if ( (InChars[0] == '\xFE') && (InChars[1] == '\xFF') ) // UTF-16BE
	{
	}
	else if ( (InChars[0] == '\xFF') && (InChars[1] == '\xFE') ) // UTF-16LE
	{
	}
	else //Assume UTF-8
	{
		if ( (InChars[0] == '\xEF') && (InChars[1] == '\xBB') && (InChars[2] == '\xBF') ) // Explicit UTF-8
			InChars += 3;

		return ParseUTF8(InChars, CStrlen(InChars));
	}

	// Keep garbage data in case of failure
	return false;
}


inline bool CParserUTF::ParseUTF8( const char* InputData, size_t InputLength)
{
	size_t ExpectedLength = utf8::DecodedLen<wchar_t>(InputData);
	if ( ExpectedLength > 0 )
	{
		OutputData = CScopeMem( (ExpectedLength+1) * sizeof(wchar_t) );
		Output = OutputData.GetArray<wchar_t>();
		Length = utf8::Decode(OutputData.GetArray<wchar_t>(), ExpectedLength+1, InputData);
		return Length == ExpectedLength;
	}
	return false;
}
