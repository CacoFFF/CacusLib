/*=============================================================================
	Parser/XML.h
	Author: Fernando Velázquez

	Simple, single allocation result, read-only XML parser.
=============================================================================*/


/*
RFC 7303
https://datatracker.ietf.org/doc/html/rfc7303


*/


class CACUS_API CParserFastXML
{
public:
	enum ENodeType
	{
		NT_Element,        // <Content></Content>
		NT_String,         // Text inside is Content
		NT_Comment,        // <!--Content-->
	};

	// Post-parse XML Tree
	template <typename CHAR> struct Attrib
	{
		Attrib<CHAR>* Next;
		const CHAR*   Name;
		const CHAR*   Value;
	};

	template <typename CHAR> struct Node
	{
		Node<CHAR>*   Next;
		Node<CHAR>*   Child;         // Valid if Type==NT_Element
		const CHAR*   Content;
		Attrib<CHAR>* Attribute;     // Valid if Type==NT_Element
		int_p         Type;
	};

	CParserFastXML();
	~CParserFastXML();

	bool Parse( const wchar_t* Input);
	void Empty();

	// Simplified browser helper
	class Browser
	{
	public:
		Browser( const Node<wchar_t>* InPosition);

		bool Start( const wchar_t* FirstElementName);
		bool Next( const wchar_t* NextElementName);
		bool Down( const wchar_t* ChildElementName);

		operator bool();
		CACUS_API const wchar_t* operator*(); // Outputs contents (excluding Element brackets) using the circular buffer.

		const Node<wchar_t>* Position;
	};

	Browser CreateBrowser()  { return Browser(Root); }


protected:
	void* Data;

public:
	Node<wchar_t>* Root;
};


/*----------------------------------------------------------------------------
	CParserFastXML inlines
----------------------------------------------------------------------------*/

inline CParserFastXML::CParserFastXML()
	: Data(nullptr)
	, Root(nullptr)
{
}


/*----------------------------------------------------------------------------
	CParserFastXML::Iterator inlines
----------------------------------------------------------------------------*/

inline CParserFastXML::Browser::Browser( const CParserFastXML::Node<wchar_t>* InPosition)
	: Position(InPosition)
{
}

inline bool CParserFastXML::Browser::Start( const wchar_t* FirstElementName)
{
	for ( auto* Link=Position; Link; Link=Link->Next)
		if ( (Link->Type == NT_Element) && Link->Content && !CStricmp(Link->Content,FirstElementName) )
		{
			Position = Link;
			return true;
		}

	Position = nullptr;
	return false;
}

inline bool CParserFastXML::Browser::Next( const wchar_t* NextElementName)
{
	if ( Position )
	{
		for ( auto* Link=Position->Next; Link; Link=Link->Next)
			if ( (Link->Type == NT_Element) && Link->Content && !CStricmp(Link->Content,NextElementName) )
			{
				Position = Link;
				return true;
			}
		Position = nullptr;
	}
	return false;
}

inline bool CParserFastXML::Browser::Down( const wchar_t* ChildElementName)
{
	if ( Position )
	{
		for ( auto* Link=Position->Child; Link; Link=Link->Next)
			if ( (Link->Type == NT_Element) && Link->Content && !CStricmp(Link->Content,ChildElementName) )
			{
				Position = Link;
				return true;
			}
		Position = nullptr;
	}
	return false;
}

inline CParserFastXML::Browser::operator bool()
{
	return Position != nullptr;
}

