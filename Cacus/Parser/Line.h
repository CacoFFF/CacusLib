/*=============================================================================
	Parser/Line.h
	Author: Fernando Velázquez

	Simple line parser from text streams.

	Line breaks are checked from first to last and may be respecified.
	Default line breaks are (in the following order):
	- "\r\n" "\n" "\r"
	Also, "\0" is always a line break

	Notes:
	- GetLineArray() will have an additional nullptr entry at Index=LineCount
	for algorithms that don't want to query GetLineCount()
	- Line breaks may be replaced with tokens for a token based parser.
=============================================================================*/


//
// 8 bit char line parser.
//
class CACUS_API CParserFastLine8
{
protected:
	size_t       LineCount;
	const char** LineArray;
	void*        Data;

	size_t       LineBreakCount;
	const char** LineBreakArray;
public:
	CParserFastLine8();
	~CParserFastLine8();

	void SetLineBreaks( const char** NewLineBreakArray, size_t NewLineBreakCount);

	bool Parse( const char* Text);

	// Inlines
	size_t       GetLineCount();
	const char** GetLineArray();
};


/*-----------------------------------------------------------------------------
	CParserFastLine8.
-----------------------------------------------------------------------------*/

inline size_t CParserFastLine8::GetLineCount()
{
	return LineCount;
}

inline const char** CParserFastLine8::GetLineArray()
{
	return LineArray;
}
