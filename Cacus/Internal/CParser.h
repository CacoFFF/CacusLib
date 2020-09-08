#pragma once

#ifdef _STRING_
// Manually parse text using this if the stock code doesn't work for you
class CStruct;
class CProperty;
enum EParseType;

enum EParserElementFlags
{
	PEF_Null          = 0x0001,
	PEF_StringQuoted  = 0x0002,
	PEF_Object        = 0x0004,
	PEF_Array         = 0x0008,
	PEF_DeltaSame     = 0x0010, //Marked as 'same' by delta check
	PEF_Root          = 0x0020, //Should not delete
	PEF_DeltaExport   = 0x0040,
	PEF_Inner         = 0x0080, //Keep?

	PEF_Inherit       = PEF_DeltaExport,
};

class CParserElement
{
public:
	CParserElement* Next;
	CParserElement* Children;
	std::string Key;
	std::string Value;
	const char* TypeName;
	int32 Flags;

	//Default constructor
	CParserElement( int32 InFlags)
		: Next(nullptr), Children(nullptr), TypeName(nullptr), Flags(InFlags) {}

	//Child constructor (attach at front of linked list)
	CParserElement( CParserElement& Parent, const char* InKeyName="")
		: Next(Parent.Children), Children(nullptr), Key(InKeyName), TypeName(nullptr), Flags(Parent.Flags&PEF_Inherit)
	{ Parent.Children = this; }

	~CParserElement();

	void LogTokens( uint32 Depth=0);
	uint32 CountTokens( const bool bCountChildren=false);

	CParserElement* GetChild( const char* ChildKey) const;
	CParserElement* GetChild( const char* ChildKey, bool bCreate=false);
	CParserElement* GetChild( uint32 ChildId, bool bCreate=false);
	CParserElement* AddChildLast( CParserElement* NewChild); //TODO: Template-ize

	void ExportAs( std::string& String, EParseType ExportType);
	void Export_JSONString( std::string& String);

	void ParseObject( const CStruct* Struct, void*& Into) const;
	void ParseObjects( const CStruct* Struct, CArray<void*>& Into) const; //Overwrites elements if existing, does not deallocate excedent
	void ExportObject( const CStruct* Struct, void* From, bool Root=true);

	template<class T> T* ParseObject()                   { return (T*)ParseObject( T::GetInstanceCStruct()); }
	template<class T> T* ParseObject( T* Into)           { return (T*)ParseObject( T::GetInstanceCStruct(), Into); }
	template<class T> void ParseObjects( CArray<T*>& IO) { ParseObjects( T::GetInstanceCStruct(), *(CArray<void*>*)&IO); }

	bool operator==( const CParserElement& Other) const;
	bool operator!=( const CParserElement& Other) const  { return !(*this == Other); }

	static uint32 RemoveFlaggedTokens( CParserElement** TokenList, int32 Flags);
};



class CParser
{
public:
	const char* Data;
	CParserElement RootElement;

	CParser( const char* InData=nullptr)  : Data(InData), RootElement(PEF_Root) {}
	virtual ~CParser()                    {}
	virtual bool Parse()                  { return false; }
};

class CJSONParser : public CParser
{
public:
	CJSONParser( const char* InData)  : CParser(InData) {}
	bool Parse();
	bool ParseKey( CParserElement* Parent);
	bool ParseValue( CParserElement* Element);
	bool ParseObject( CParserElement* Element);
	bool ParseArray( CParserElement* Container);
};

class CPlainTextFormParser : public CParser
{
	const char* Line;
public:
	CPlainTextFormParser( const char* InData) : CParser(InData), Line(nullptr) {}
	bool Parse();
	bool ParseMembers( CParserElement* Parent); //Root line parser
	bool ParseArray( CParserElement* Container);
};

#endif
