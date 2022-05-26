/*=============================================================================
	Parser/XML.cpp:
	Fast XML parser implementation for Cacus library.
	Author: Fernando Velázquez

	https://tools.ietf.org/html/rfc7303
	This XML parser runs various validation steps before parsing the document.

	Resources:
	https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form
	https://www.liquid-technologies.com/XML/EBNF1.0.aspx
	https://www.liquid-technologies.com/XML/EBNF1.1.aspx

=============================================================================*/

/*

&lt; 	< 	less than
&gt; 	> 	greater than
&amp; 	& 	ampersand 
&apos; 	' 	apostrophe
&quot; 	" 	quotation mark

*/

/*
* 
< (less-than) &#60; or &lt;
> (greater-than) &#62; or  &gt;
& (ampersand) &#38;
' (apostrophe or single quote) &#39;
" (double-quote) &#34;

*/


#include "../CacusLibPrivate.h"

#include "CacusString.h"
#include "CacusMem.h"
#include "Parser/Base.h"
#include "Parser/XML.h"

#include <tuple>


template <typename CHAR> const CHAR* _text_inner( const char* T8, const wchar_t* TW, const char16* T16, const char32* T32);
template <> const char*    _text_inner<char>   ( const char* T8, const wchar_t* TW, const char16* T16, const char32* T32) {return T8;}
template <> const wchar_t* _text_inner<wchar_t>( const char* T8, const wchar_t* TW, const char16* T16, const char32* T32) {return TW;}
template <> const char16*  _text_inner<char16> ( const char* T8, const wchar_t* TW, const char16* T16, const char32* T32) {return T16;}
template <> const char32*  _text_inner<char32> ( const char* T8, const wchar_t* TW, const char16* T16, const char32* T32) {return T32;}
#define _text(TYPE,T) _text_inner<TYPE>(T,L##T,u##T,U##T)

// XML tree parsing helpers
namespace XMLParser
{
	static const char* Stage = "";

	// Used to represent a string
	template <typename CHAR> struct String
	{
		const CHAR* Start;
		const CHAR* End;

		String()
			: Start(nullptr)
			, End(nullptr)
		{}

		bool ParseElementName( const CHAR*& Stream);
		bool ParseElementText( const CHAR*& Stream);
		bool ParseAttributeName( const CHAR*& Stream);
		bool ParseAttributeValue( const CHAR*& Stream);
		bool ProcessSpecialCharacters( CMemExStack& Mem);

		size_t Length()       { return (size_t) (End - Start); }
		size_t OutputMemory() { return (Length()+1) * sizeof(CHAR); }
		CHAR* OutputToBlock( CHAR*& StringBlock);


		bool operator==( const String<CHAR>& Other)
		{
			return (End-Start == Other.End-Other.Start) && !CStrncmp(Start, Other.Start, End-Start);
		}

		bool operator!=( const String<CHAR>& Other)
		{
			return (End-Start != Other.End-Other.Start) || CStrncmp(Start, Other.Start, End-Start);
		}
	};

	// Used to represent an attribute
	template <typename CHAR> struct Attribute
	{
		Attribute<CHAR>* Next;
		String<CHAR>*    Key;
		String<CHAR>*    Value;

		Attribute()
			: Next(nullptr)
		{}
	};

	// Used to represent an element and keep track of tree structure
	template <typename CHAR> struct Element
	{
		Element<CHAR>*   Next;
		Element<CHAR>*   Parent;
		Element<CHAR>*   Child;
		Element<CHAR>**  ChildLinkPtr;
		String<CHAR>*    ElementName;
		String<CHAR>*    ElementText;
		Attribute<CHAR>* Attribute;
		int_p            Open;

		Element()
			: Next(nullptr)
			, Parent(nullptr)
			, Child(nullptr)
			, ChildLinkPtr(&Child)
			, ElementName(nullptr)
			, ElementText(nullptr)
			, Attribute(nullptr)
		{}

		bool AttachTo( Element<CHAR>* NewParent);
		void ParseAttributes( const CHAR*& Stream, CMemExStack& Mem);
	};

	template <typename CHAR> bool IsAttributeSeparator( const CHAR C);
	template <typename CHAR> uint32 ParseDecCharacter( const CHAR* Token);
	template <typename CHAR> uint32 ParseHexCharacter( const CHAR* Token);
};


/*----------------------------------------------------------------------------
	CParserFastXML
----------------------------------------------------------------------------*/

CParserFastXML::~CParserFastXML()
{
	Empty();
}

void CParserFastXML::Empty()
{
	if ( Data )
	{
		CFree(Data);
		Data = nullptr;
	}
	Root = nullptr;
}


/*----------------------------------------------------------------------------
	CParserFastXML::Browser
----------------------------------------------------------------------------*/

struct RecursiveNodeStringBuilder
{
	size_t Length;
	wchar_t* Output;
	wchar_t* WritePosition;

	const wchar_t* GetString( const CParserFastXML::Node<wchar_t>* BaseNode)
	{
		if ( BaseNode->Type == CParserFastXML::NT_Comment || BaseNode->Type == CParserFastXML::NT_String )
			return BaseNode->Content ? BaseNode->Content : L"";

		if ( (BaseNode->Type == CParserFastXML::NT_Element) && BaseNode->Child )
		{
			Length = 0;
			CountCharsInnerRecurse(BaseNode->Child);
			Output = (wchar_t*)CStringBuffer( (Length+1)*sizeof(wchar_t) );
			if ( Output )
			{
				WritePosition = Output;
				PrintCharsInnerRecurse(BaseNode->Child);
				*WritePosition = '\0';
				return Output; // TODO: Ensure (WritePosition-Output) is Length
			}
		}

		return L"";
	}


private:
	//
	// Note: content-less nodes are intentionally skipped
	//
	void CountCharsInnerRecurse( const CParserFastXML::Node<wchar_t>* Node)
	{
		for ( ; Node; Node=Node->Next)
			if ( Node->Content )
			{
				if ( Node->Type == CParserFastXML::NT_Element )
				{
					Length += CStrlen(Node->Content) * 2 + _len("<></>"); // Extra chars!
					if ( Node->Child )
						CountCharsInnerRecurse(Node->Child);
				}
				else if ( Node->Type == CParserFastXML::NT_String )
					Length += CStrlen(Node->Content);
			}
	}

	void PrintCharsInnerRecurse( const CParserFastXML::Node<wchar_t>* Node)
	{
		for ( ; Node; Node=Node->Next)
			if ( Node->Content )
			{
				if ( Node->Type == CParserFastXML::NT_Element )
				{
					*this << L"<" << Node->Content << L">";
					if ( Node->Child )
						PrintCharsInnerRecurse(Node->Child);
					*this << L"</" << Node->Content << L">";
				}
				else if ( Node->Type == CParserFastXML::NT_String )
					*this << Node->Content;
			}
	}

	RecursiveNodeStringBuilder& operator<<(const wchar_t* In)
	{
		wchar_t* Out = WritePosition;
		while ( *In )
			*Out++ = *In++;
		WritePosition = Out;
		return *this;
	}
};

const wchar_t* CParserFastXML::Browser::operator*()
{
	RecursiveNodeStringBuilder Builder;
	return Builder.GetString(Position);
}


/*----------------------------------------------------------------------------
	CParserFastXML internals.
----------------------------------------------------------------------------*/

template <typename CHAR> void LogElements( const XMLParser::Element<CHAR>& Element, int Level=0)
{
}
template <> void LogElements<wchar_t>( const XMLParser::Element<wchar_t>& Element, int Level)
{
	for ( int i=0; i<=Level; i++)
		wprintf(L"=");
	if ( Element.ElementName )
	{
		wprintf(L"<%s>", CopyToBuffer(Element.ElementName->Start,Element.ElementName->Length()) );
		if ( Element.Child && Element.Child->ElementText && !Element.Child->Next ) // Inline instead
		{
			wprintf(L": %s\r\n", CopyToBuffer(Element.Child->ElementText->Start,Element.Child->ElementText->Length()) );
			return;
		}
	}
	if ( Element.ElementText )
		wprintf(L" -- %s", CopyToBuffer(Element.ElementText->Start,Element.ElementText->Length()) );
	wprintf(L"\r\n");
	for ( auto* Child=Element.Child; Child; Child=Child->Next)
		LogElements(*Child, Level+1);
}
template <> void LogElements<char>( const XMLParser::Element<char>& Element, int Level)
{
}

template <typename CHAR> bool templ_parse( const CHAR*& Stream, XMLParser::Element<CHAR>*& RootElement, CMemExStack& MemTree)
{
	static const CHAR* SkipChars       = _text(CHAR," \r\n	");
	static const CHAR* PostElementSkip = _text(CHAR,"\r\n");
	AdvanceThrough(Stream, SkipChars);


	// Optional XML declaration
	// Ugly handling
	if ( !CStrnicmp(Stream, _text(CHAR,"<?xml"), 5) )
	{
		Stream += 5;
		AdvanceThrough(Stream, SkipChars);

		while ( *Stream != '\0' && *Stream != '>' )
		{
/*			if ( *Stream == '\x22' )
			{	do{ Stream++ } while( *Stream!='\x22'); }
			else if ( *Stream == '\x27' )
			{	do{ Stream++ } while( *Stream!='\x27'); }*/ //BUGGED
//			else if //ALNUM KEYWORD PARSING!!
//			else
				Stream++;
		}
		if ( *Stream == '>' )
			Stream++;
	}

	AdvanceThrough(Stream, SkipChars);
	if ( *Stream != '<' )
		return false;

	XMLParser::Element<CHAR>*  CurElement     = nullptr;
	do
	{
		// Element definition begin
		if ( EatChar(Stream,'<') )
		{
			// Closing an Element
			if ( EatChar(Stream,'/') )
			{
				if ( !CurElement || !CurElement->ElementName ) // Error
					break;

				XMLParser::Stage = "Close Element Statement";
				XMLParser::String<CHAR> ClosingName;
				if ( !ClosingName.ParseElementName(Stream) || ClosingName != *CurElement->ElementName || !EatChar(Stream,'>') ) // Error
					break;
				// SKIP SPACES BEFORE '>' ?
				CurElement = CurElement->Parent;
				AdvanceThrough(Stream, PostElementSkip);
				continue;
			}

			// SKIP SPACES HERE?
			XMLParser::Stage = "Open Element Statement";
			XMLParser::String<CHAR> OpeningName;
			if ( !OpeningName.ParseElementName(Stream) ) // Error
				break;

			XMLParser::Element<CHAR>* NewElement = new(MemTree) XMLParser::Element<CHAR>;
			NewElement->ElementName = new(MemTree) XMLParser::String<CHAR>(OpeningName);
			NewElement->ParseAttributes(Stream, MemTree);
			if ( !NewElement->AttachTo(CurElement) )
				RootElement = NewElement; // No parent means New is Root

			// Element is not being immediately closed, traverse down tree
			XMLParser::Stage = "Close Element Statement";
			if ( EatChar(Stream,'>') )
				CurElement = NewElement;
			else if ( !EatChar(Stream,'/') || !EatChar(Stream,'>') ) // Error
				break;
			AdvanceThrough(Stream, PostElementSkip);
		}
		// Error
		else if ( !RootElement )
			break;
		// String element
		else
		{
			XMLParser::Element<CHAR>* NewStringElement = new(MemTree) XMLParser::Element<CHAR>;
			NewStringElement->ElementText = new(MemTree) XMLParser::String<CHAR>;
			if ( !NewStringElement->ElementText->ParseElementText(Stream) ) // Error
				break;
			NewStringElement->AttachTo(CurElement);
		}
	} while ( CurElement != nullptr );

	// Parse successful if escapes root element
	return (RootElement != nullptr) && (CurElement == nullptr);
}


template <typename CHAR> void templ_preprocess_elements( CMemExStack& Mem, const XMLParser::Element<CHAR>* RootElement, size_t& TotalElements, size_t& TotalAttributes, size_t& TotalStringMemory)
{
	// Traverse all elements
	TotalElements = 0;
	TotalAttributes = 0;
	TotalStringMemory = 0;
	const XMLParser::Element<CHAR>* Element = RootElement;
	while ( Element )
	{
		TotalElements++;
		if ( Element->ElementName )
		{
			Element->ElementName->ProcessSpecialCharacters(Mem);
			TotalStringMemory += Align(Element->ElementName->OutputMemory(),EALIGN_PLATFORM_PTR);
		}
		if ( Element->ElementText )
		{
			Element->ElementText->ProcessSpecialCharacters(Mem);
			TotalStringMemory += Align(Element->ElementText->OutputMemory(),EALIGN_PLATFORM_PTR);
		}

		// Traverse all attributes
		for ( const XMLParser::Attribute<CHAR>* Attribute=Element->Attribute; Attribute; Attribute=Attribute->Next)
		{
			TotalAttributes++;
			Attribute->Key->ProcessSpecialCharacters(Mem);
			Attribute->Value->ProcessSpecialCharacters(Mem);
			TotalStringMemory += Align(Attribute->Key->OutputMemory(),EALIGN_PLATFORM_PTR);
			TotalStringMemory += Align(Attribute->Value->OutputMemory(),EALIGN_PLATFORM_PTR);
		}

		if ( Element->Child )
			Element = Element->Child;
		else if ( Element->Next )
			Element = Element->Next;
		else
		{
			traverse_parent:
			Element = Element->Parent;
			if ( !Element )
				break;
			if ( !Element->Next )
				goto traverse_parent;
			Element = Element->Next;
		}
	}
}

template <typename CHAR> void templ_process_elements( const XMLParser::Element<CHAR>* RootElement, CParserFastXML::Node<CHAR>* NodeArray, CParserFastXML::Attrib<CHAR>* AttributeArray, CHAR* StringBlock)
{
	size_t NodeCount = (const XMLParser::Element<CHAR>*)NodeArray - RootElement; //Ugly, but gives Root count
	struct ParentLink
	{
		ParentLink* Parent;
		CParserFastXML::Node<CHAR>* Node;
	};
	CScopeMem ParentLinkMem(NodeCount * sizeof(ParentLink));
	ParentLink* ParentLinks = ParentLinkMem.GetArray<ParentLink>();
	ParentLinks[0].Parent = nullptr;
	ParentLinks[0].Node   = NodeArray;

	// Traverse all elements
	const XMLParser::Element<CHAR>* Element = RootElement;
	while ( Element )
	{
		NodeArray->Child  = nullptr;
		NodeArray->Next   = nullptr;
		if ( Element->ElementName )
		{
			NodeArray->Type = CParserFastXML::NT_Element;
			NodeArray->Content = Element->ElementName->OutputToBlock(StringBlock);
		}
		else if ( Element->ElementText )
		{
			NodeArray->Type = CParserFastXML::NT_String;
			NodeArray->Content = Element->ElementText->OutputToBlock(StringBlock);
		}

		// Traverse all attributes
		CParserFastXML::Attrib<CHAR>** AttribPtr = &NodeArray->Attribute;
		for ( const XMLParser::Attribute<CHAR>* Attribute=Element->Attribute; Attribute; Attribute=Attribute->Next)
		{
			*AttribPtr = AttributeArray;
			AttributeArray->Name  = Attribute->Key->OutputToBlock(StringBlock);
			AttributeArray->Value = Attribute->Value->OutputToBlock(StringBlock);
			AttribPtr = &AttributeArray->Next;
			AttributeArray++;
		}
		*AttribPtr = nullptr;
		NodeArray++;

		if ( Element->Child )
		{
			Element = Element->Child;
			ParentLinks[1].Parent = ParentLinks;
			ParentLinks[1].Node   = NodeArray;
			ParentLinks[0].Node->Child = NodeArray;
			ParentLinks++;
		}
		else if ( Element->Next )
		{
			Element = Element->Next;
			ParentLinks[0].Node->Next = NodeArray;
			ParentLinks[0].Node = NodeArray;
		}
		else
		{
		traverse_parent:
			Element = Element->Parent;
			ParentLinks = ParentLinks[0].Parent;
			if ( !Element )
				break;
			if ( !Element->Next )
				goto traverse_parent;
			Element = Element->Next;
			ParentLinks[0].Node->Next = NodeArray;
			ParentLinks[0].Node = NodeArray;
		}
	}
}





bool CParserFastXML::Parse( const wchar_t* Input)
{
	Empty();

	// This stack contains all pointers to original stream data
	CMemExStack TreeStack(4096);

	XMLParser::Element<wchar_t>* RootElement = nullptr;

	const wchar_t* Stream = (const wchar_t*)Input;
	if ( !templ_parse(Stream, RootElement, TreeStack) )
	{
		printf("Stream parse error at %s\r\n\r\n", XMLParser::Stage);
		return false;
	}

	size_t Elements, Attributes, StringMemory;
	templ_preprocess_elements(TreeStack, RootElement, Elements, Attributes, StringMemory);

	if ( RootElement )
		LogElements(*RootElement);

	// Prepare single allocation block.
	size_t NodeMemory   = Elements   * sizeof(CParserFastXML::Node<wchar_t>);
	size_t AttribMemory = Attributes * sizeof(CParserFastXML::Attrib<wchar_t>);
	Data = CMalloc(NodeMemory + AttribMemory + StringMemory + 32);

	CParserFastXML::Node<wchar_t>*   NodeArray   = AddressOffset<CParserFastXML::Node<wchar_t>>(Data, 0);
	CParserFastXML::Attrib<wchar_t>* AttribArray = AddressOffset<CParserFastXML::Attrib<wchar_t>>(NodeArray, NodeMemory);
	wchar_t*                         StringBlock = AddressOffset<wchar_t>(AttribArray, AttribMemory);
	templ_process_elements(RootElement, NodeArray, AttribArray, StringBlock);

	for ( size_t i=0; i<Elements; i++)
	{
		wprintf(L"Node contents %i: %s\r\n", (int)i, NodeArray[i].Content);
		for ( auto* AttribLink=NodeArray[i].Attribute; AttribLink; AttribLink=AttribLink->Next)
			wprintf(L"With attribute %s = %s\r\n", AttribLink->Name, AttribLink->Value);
	}

	if ( Elements > 0 )
		Root = NodeArray;

	return true;
}



//*****************************************************
//*************** String Parse helpers ****************
//*****************************************************

template <typename CHAR>
bool XMLParser::String<CHAR>::ParseElementName( const CHAR *& Stream)
{
	const CHAR* _Start = Stream;
	CHAR C;
	while ( (C=*Stream) != '\0' )
	{
		if ( C == ' ' || C == '/' || C == '>' )
		{
			if ( Stream == _Start )
				break;
			Start = _Start;
			End   = Stream;
			return true;
		}
		// TODO: COMPLETE
		if ( C == '=' || C == '?' )
			break;
		Stream++;
	}
	return false;
}

template <typename CHAR>
bool XMLParser::String<CHAR>::ParseElementText( const CHAR*& Stream)
{
	const CHAR* _Start = Stream;
	CHAR C;
	while ( (C=*Stream) != '\0' )
	{
		if ( C == '<' )
		{
			Start = _Start;
			End   = Stream;
			return true;
		}
		// TODO: COMPLETE
		if ( C == '>' )
			break;
		Stream++;
	}
	return false;
}

template <typename CHAR>
bool XMLParser::String<CHAR>::ParseAttributeName( const CHAR*& Stream)
{
	while ( *Stream == ' ' )
		Stream++;

	const CHAR* _Start = Stream;
	CHAR C;
	while ( (C=*Stream) != '\0' )
	{
		if ( C == '=' )
		{
			if ( Stream == _Start )
				break;
			Start = _Start;
			End   = Stream;
			Stream++; // Skip '='
			return true;
		}
		// TODO: COMPLETE
		if ( C == '>' || C == '/' )
			break;
		Stream++;
	}
	return false;
}

template<typename CHAR>
bool XMLParser::String<CHAR>::ParseAttributeValue( const CHAR *& Stream)
{
	while ( *Stream == ' ' )
		Stream++;

	// Attributes are always quoted
	const CHAR QuoteChar = *Stream;
	if ( QuoteChar != '\x22' && QuoteChar != '\x27' )
		return false;

	const CHAR* _Start = Stream++;
	CHAR C;
	while ( (C=*Stream) != '\0' )
	{
		if ( C == QuoteChar )
		{
			Start = _Start;
			End   = Stream;
			Stream++; // Skip Quote End
			return true;
		}
		Stream++;
	}
	return false;
}


template<typename CHAR>
CHAR* XMLParser::String<CHAR>::OutputToBlock( CHAR*& StringBlock)
{
	CHAR* Result = StringBlock;
	CHAR* Output = Result;

	// Copy String
	size_t CopySize = Length() * sizeof(CHAR);
	if ( CopySize > 0 )
	{
		CMemcpy(Output, Start, CopySize);
		Output = AddressOffset<CHAR>(Output, CopySize);
	}

	// Fill remainder with zeroes (null terminator is added here)
	CHAR* OutputEnd = AddressAlign(Output+1, EALIGN_PLATFORM_PTR);
	while ( Output < OutputEnd )
		*Output++ = '\0';

	StringBlock = OutputEnd;
	return Result;
}


template<typename CHAR>
uint32 XMLParser::ParseDecCharacter( const CHAR* Token)
{
	// Fail if not digit
	uint32 N = 0;
	while ( *Token >= '0' && *Token <= '9' )
	{
		N = N * 10 + *Token - '0';
		if ( N > 0x1FFFF ) // ?
			return 0;
		Token++;
	}
	return (*Token == ';') ? N : 0;
}

template<typename CHAR>
uint32 XMLParser::ParseHexCharacter( const CHAR* Token)
{
	// Fail if not digit
	uint32 N = 0;
next:
	if ( N > 0x1FFFF ) // ?
		return 0;
	if ( *Token >= '0' && *Token <= '9' )
	{
		N = N * 16 + *Token - '0';
		Token++;
		goto next;
	}
	if ( *Token >= 'a' && *Token <= 'f' )
	{
		N = N * 16 + 10 + *Token - 'a';
		Token++;
		goto next;
	}
	if ( *Token >= 'A' && *Token <= 'F' )
	{
		N = N * 16 + 10 + *Token - 'A';
		Token++;
		goto next;
	}
	return (*Token == ';') ? N : 0;
}


template <typename CHAR>
bool XMLParser::String<CHAR>::ProcessSpecialCharacters( CMemExStack& Mem)
{
	// Reader, attempt to find special chars
	const size_t Len = Length();
	if ( CStrnchr(Start, '&', Len+1) )
	{
		CHAR* NewStart = (CHAR*)Mem.PushBytes(Length());
		CHAR* NewEnd   = NewStart;

		const CHAR* Stream = Start;
		while ( Stream < End )
		{
			if ( (*Stream == '&') && (Stream + 3 < End) )
			{
				// Ampersand and next 3 chars are valid
				const CHAR* TokenEnd = CStrnchr(Stream,';',End-Stream);
				const CHAR* OtherAmp = CStrnchr(Stream+1,'&',End-(Stream+1));
				if ( TokenEnd && (!OtherAmp || OtherAmp > TokenEnd) )
				{
					// Copy token with zero-termination into target memory
					CHAR* WriteDest = NewEnd;
					while ( Stream <= TokenEnd )
						*WriteDest++ = *Stream++;
					*WriteDest = '\0';

					if ( NewEnd[1] == '#' )
					{
						// Decimal or Hexadecimal representation
						uint32 C = (NewEnd[2] == 'x') ? XMLParser::ParseHexCharacter(NewEnd+3) : XMLParser::ParseDecCharacter(NewEnd+2);
						if ( C )
						{
							*NewEnd++ = (CHAR)C;
							continue;
						}
					}
#define ELSE_TRY_PARSE(input,output) \
					else if ( !CStricmp(NewEnd+1,_text(CHAR,input)) ) \
					{ \
						*NewEnd++ = output; \
						continue; \
					}
					ELSE_TRY_PARSE("amp;",'&')
					ELSE_TRY_PARSE("lt;",'<')
					ELSE_TRY_PARSE("gt;",'>')
					ELSE_TRY_PARSE("apos;",'\x27')
					ELSE_TRY_PARSE("quot;",'\x22')
#undef ELSE_TRY_PARSE
					// Nothing was converted, keep token un-parsed
					NewEnd = WriteDest;
					continue;
				}
			}
			*NewEnd++ = *Stream++;
		}
		Start = NewStart;
		End   = NewEnd;
	}
	return true;


}


//*****************************************************
//*************** Element Parse helpers ***************
//*****************************************************

template <typename CHAR>
bool XMLParser::Element<CHAR>::AttachTo( Element<CHAR>* NewParent)
{
	if ( NewParent )
	{
		Parent = NewParent;
		*NewParent->ChildLinkPtr = this;
		NewParent->ChildLinkPtr = &this->Next;
		return true;
	}
	return false;
}

template <typename CHAR>
void XMLParser::Element<CHAR>::ParseAttributes( const CHAR*& Stream, CMemExStack& Mem)
{
	XMLParser::Stage = "Element Attribute Parser";
	XMLParser::Attribute<CHAR>** AttributePtr = &Attribute;
	while ( XMLParser::IsAttributeSeparator(*Stream) )
	{
		do
		{
			Stream++;
		} while ( XMLParser::IsAttributeSeparator(*Stream) );

		// End of attribute list
		if ( *Stream == '<' || *Stream == '>' || *Stream == '/' )
			break;

		// Parse first, then use memory stack
		XMLParser::String<CHAR> _key;
		XMLParser::String<CHAR> _value;
		if ( _key.ParseAttributeName(Stream) && _value.ParseAttributeValue(Stream) )
		{
			XMLParser::Attribute<CHAR>* NewAttribute = new(Mem) XMLParser::Attribute<CHAR>();
			NewAttribute->Key   = new(Mem) XMLParser::String<CHAR>(_key);
			NewAttribute->Value = new(Mem) XMLParser::String<CHAR>(_value);
			*AttributePtr = NewAttribute;
			AttributePtr = &NewAttribute->Next;
		}
	}
}

//*****************************************************
//*************** Global Parser helpers ***************
//*****************************************************

template <typename CHAR>
inline bool XMLParser::IsAttributeSeparator( const CHAR C)
{
	return C == ' ' 
		|| C == '\r'
		|| C == '\n'
		|| C == '\x9';
}
