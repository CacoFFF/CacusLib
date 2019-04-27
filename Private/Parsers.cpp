
#include "CacusOutputDevice.h"
#include "CacusField.h"
#include "StackUnwinder.h"
#include "DebugCallback.h"


//*******************************************************************
// PARSER ROOT

#include "Internal/CParser.h"
#define JSON_SKIP " \r\n"

#ifdef _STRING_

static CParser* CreateParser( const char* Data, EParseType ParseType)
{
	switch (ParseType)
	{
	case PARSE_PlainTextForm:
		return new CPlainTextFormParser(Data);
	case PARSE_JSON:
		return new CJSONParser(Data);
	default:
		return new CParser(Data);
	}
}

void CParserElement::ExportAs( std::string& String, EParseType ExportType)
{
	switch (ExportType)
	{
	case PARSE_JSON:
		Export_JSONString(String);
		break;
	default:
		break;
	}	
}

bool DataToObject( const char* Data, void* Into, CProperty* Outer, EParseType ImportType)
{
	CParser* Parser = CreateParser( Data, ImportType);
	bool Result = Parser->Parse();
	if ( Result )
		Outer->Import( Into, Parser->RootElement);
	delete Parser;
	return Result;
}

void ObjectToData( const char*& OutData, void* From, CProperty* Outer, EParseType ExportType, int Delta)
{
	CParserElement ElementList(PEF_Root);
	if ( Delta )
		ElementList.Flags |= PEF_DeltaExport;
	Outer->Export( From, ElementList );

	std::string StrBuffer;
	ElementList.ExportAs( StrBuffer, ExportType);

	size_t Len = StrBuffer.length();
	if ( Len )
	{
		char* Data = CharBuffer<char>( Len + 1 );
		CMemcpy( Data, StrBuffer.data(), Len+1 );
		Data[Len] = 0;
		OutData = Data;
	}
	else
		OutData = "";
}

bool DataToData( const char* InData, const char*& OutData, EParseType InFormat, EParseType OutFormat)
{
	CParser* Parser = CreateParser( InData, InFormat);
	bool Result = Parser->Parse();
	OutData = "";
	if ( Result )
	{
		std::string StrBuffer;
		StrBuffer.reserve( 2048);
		Parser->RootElement.ExportAs( StrBuffer, OutFormat);
		//Manual buffer, as we know LEN already
		size_t Len = StrBuffer.length();
		if ( Len )
		{
			char* Data = CharBuffer<char>( Len + 1 );
			CMemcpy( Data, StrBuffer.data(), Len+1 );
			Data[Len] = 0;
			OutData = Data;
		}
	}
	delete Parser;
	return Result;
}

void ObjectToObject( void* From, void* Into, CProperty* FromOuter, CProperty* IntoOuter)
{
	CParserElement ElementList(PEF_Root);
	FromOuter->Export( From, ElementList);
	IntoOuter->Import( Into, ElementList);
}


//*******************************************************************
// PARSER ELEMENT

CParserElement::~CParserElement()
{
	LinkedListDelete(Children);
	LinkedListDelete(Next);
}

void CParserElement::LogTokens( uint32 Depth)
{
	uint32 i;
	CParserElement* Element = this;
	while ( Element )
	{
		std::string Text;
		for ( i=0 ; i<Depth ; i++ )
			Text += "  ";
		Text += "Key " + Element->Key;
		if ( Element->Value.length() > 0 )
			Text += " = " + Element->Value;
		if ( Element->Flags & PEF_Array )
			Text += " ARRAY";
		if ( Element->Flags & PEF_Object )
			Text += " OBJECT";
		DebugCallback( Text.c_str(), CACUS_CALLBACK_PARSER );
		Element->Children->LogTokens( Depth+1);
		Element = Element->Next;
	}
}


#define ROOTLOG(...) if ( Flags & PEF_Root ) Cdebug(#__VA_ARGS__);

uint32 CParserElement::CountTokens( const bool bCountChildren)
{
	uint32 Count = 0;
	for ( CParserElement* Link=this ; Link ; Link=Link->Next )
	{
		Count++;
		if ( bCountChildren && Link->Children )
			Count += Link->Children->CountTokens(true);
	}
	return Count;
}

CParserElement* CParserElement::GetChild( const char* ChildKey) const
{
	for ( auto* Link=Children ; Link ; Link=Link->Next )
		if ( !_stricmp( Link->Key.c_str(), ChildKey) )
			return Link;
	return nullptr;
}

CParserElement* CParserElement::GetChild( const char* ChildKey, bool bCreate)
{
	for ( auto* Link=Children ; Link ; Link=Link->Next )
		if ( !_stricmp( Link->Key.c_str(), ChildKey) )
			return Link;
	return bCreate ? new CParserElement(*this,ChildKey) : nullptr;
}

CParserElement* CParserElement::GetChild( uint32 ChildId, bool bCreate) //Should be used with array
{
	uint32 i = 0;
	for ( auto** Ptr=&Children ; true ; Ptr=&(*Ptr)->Next, i++ )
	{
		if ( *Ptr == nullptr ) //Chain not long enough
		{
			if ( bCreate )
				*Ptr = new CParserElement( Flags & PEF_Inherit);
			else
				break;
		}
		if ( i == ChildId )
			return *Ptr;
	}
	return nullptr;
}

CParserElement* CParserElement::AddChildLast( CParserElement* NewChild)
{
	auto** Ptr = &Children;
	while ( *Ptr )
		Ptr = &(*Ptr)->Next;
	*Ptr = NewChild;
	return NewChild;
}

static bool JSON_NeedsStringQuote( const char* Value, size_t Len)
{
	if ( *Value == '\0' ) //No data
		return false;
	if ( CGetCharType(*Value) & CHTYPE_TokSp ) //Separator chars immediately found in value
		return true;
	size_t Offset = 0;
	ParseToken( Value, &Offset); //TODO: Create version that doesn't request data buffer (only test)
	return Offset != Len; //Token cannot be parsed without string quotes
}

void CParserElement::Export_JSONString( std::string& String)
{
	if ( Key.length() && !(Flags & PEF_Inner) )
		String += CSprintf( "\x22%s\x22:", Key.c_str() );

	if ( Flags & PEF_Object )
		String += '{';
	else if ( Flags & PEF_Array )
		String += '[';

	if ( Value.length() )
	{
		if ( (Flags & PEF_StringQuoted) || JSON_NeedsStringQuote( Value.c_str(), Value.length() ) )
			String += CSprintf( "\x22%s\x22", Value.c_str() );
		else
			String += Value;
	}
	else if ( Flags & PEF_Null )
		String += "null";
	else if ( Flags & PEF_StringQuoted )
		String += "\x22\x22";

	for ( CParserElement* Link=Children ; Link ; Link=Link->Next )
	{
		Link->Export_JSONString(String);
		if ( Link->Next )
			String += ',';
	}

	if ( Flags & PEF_Object )
		String += '}';
	else if ( Flags & PEF_Array )
		String += ']';
}

void CParserElement::ParseObject( const CStruct* Struct, void*& Into) const
{
	if ( Flags & PEF_Object )
	{
		for ( CParserElement* Elem=Children ; Elem ; Elem=Elem->Next )
			if ( _stricmp( Elem->Key.c_str(), "Class") )
			{
				CStruct* StructOther = GetStruct( Elem->Value.c_str() );
				if ( StructOther )
				{
					Struct = StructOther;
					break;
				}
			}
		if ( !Into )
			Into = (*Struct->DefaultCreator)();
		else //Check for class mismatch, if mismatch then do not import
		{
			CProperty* ClassNameProp;
			CParserElement* ClassNameElem;
			if ( (ClassNameProp=Struct->FindProperty("Class")) != nullptr
				&& (ClassNameElem=GetChild("Class")) != nullptr  //We have a class descriptor in struct and element
				&& _stricmp(ClassNameProp->Name,ClassNameElem->Key.c_str()) ) //But it's a mismatch
			{
				return; //Do not import
			}
		}
		for ( CParserElement* Elem=Children ; Elem ; Elem=Elem->Next )
		{
			CProperty* Property = Struct->FindProperty( Elem->Key.c_str() );
			if ( Property && !(Property->PropertyFlags & PF_NoImport) )
				Property->Import( Into, *Elem );
		}
		if ( Struct->PostParseFunction )
			(*Struct->PostParseFunction)(Into);
	}
}

void CParserElement::ParseObjects( const CStruct* Struct, CArray<void*>& Into) const
{
	guard(FParserElement::ParseObjects);
	if ( Flags & PEF_Array )
	{
		uint32 i=0;
		for ( CParserElement* Elem=Children ; Elem ; Elem=Elem->Next )
		{
			// Attempt to fill and go over regardless of result
			if ( i==Into.size() )
				Into.push_back(nullptr);
			Elem->ParseObject( Struct, Into[i++]);
		}
	}
	unguard;
}

void CParserElement::ExportObject( const CStruct* Struct, void* From, bool Root)
{
	if ( !From || !(Flags & PEF_Object) )
		return;

	if ( Root ) //Try to determine polymorphism
	{
		CStruct* RealStruct;
		CProperty* ClassProp;
		if ( (ClassProp = Struct->FindProperty("Class")) != nullptr
			&& (RealStruct = GetStruct(ClassProp->String(From))) != nullptr )
			Struct = RealStruct;
	}

	if ( Struct->SuperStruct )
		ExportObject( Struct->SuperStruct, From, false);
	void* Delta = (Flags & PEF_DeltaExport) ? Struct->DefaultObject : nullptr;
	for ( CProperty* Link=Struct->Properties ; Link ; Link=Link->NextProperty )
		if ( Link->ShouldExport(From, Delta) )
		{
			CParserElement* NewElement = new CParserElement( *this, Link->Name);
			Link->Export( From, *NewElement);
			//TODO: EMPTY PROPERTYSTRUCT NEEDS TO BE REMOVED (PTR VERSION NOT BECAUSE IT CALLS NEW() )
		}
}

bool CParserElement::operator==( const CParserElement & Other) const
{
	return Flags==Other.Flags
		&& Key.length() == Other.Key.length() //Visual Studio STL does not do length check on ==
		&& Value.length() == Other.Value.length()
		&& Key==Other.Key
		&& Value==Other.Value;
}

uint32 CParserElement::RemoveFlaggedTokens( CParserElement** TokenList, int32 Flags)
{
	uint32 Removed = 0;
	while ( *TokenList )
	{
		CParserElement* Token = *TokenList;
		if ( Token->Flags & Flags )
		{
			*TokenList = Token->Next;
			delete Token;
			Removed++;
		}
		else
			TokenList = &(Token->Next);
	}
	return Removed;
}


//*******************************************************************
// JSON TEXT

bool CJSONParser::Parse()
{
	AdvanceThrough( Data, JSON_SKIP);
	return 
		  (*Data == '{') ? ParseObject(&RootElement)
		: (*Data == '[') ? ParseArray( &RootElement)
		:                  false;
}

bool CJSONParser::ParseKey( CParserElement* Parent)
{
	AdvanceThrough( Data, JSON_SKIP);
	if ( *Data == '\x22' ) // " >> this is a key
	{
		size_t Position = 0;
		char* TKey = ParseToken( Data, &Position); 
		Data += Position;
		AdvanceThrough( Data, JSON_SKIP);
		if ( TKey && (*Data == ':') ) //Token and ':' needed
		{
			Data++;
			CParserElement* Element = new CParserElement(*Parent,TKey); //Properties are unordered
			return ParseValue( Element );
		}
		Cdebugf( "FJSONParser::ParseKey -> Error (Key=%s)", TKey ? TKey : "null");
	}
	return false;
}

bool CJSONParser::ParseValue( CParserElement* Element)
{
	AdvanceThrough( Data, JSON_SKIP);
	if ( *Data == '{' ) //New Object
		return ParseObject( Element);
	else if ( *Data == '[' ) //New Array
		return ParseArray( Element);
	else if ( *Data == ',' )
	{
		Cdebug( "FOUND COMMA!!!");
	}
	//Empty value
	else //Token
	{
		bool bStringToken = (*Data == '\x22');
		size_t Position = 0;
		char* TVal = ParseToken( Data, &Position);
		if ( TVal )
		{
			Data += Position;
			if ( !bStringToken && !strcmp(TVal,"null") )
				Element->Flags |= PEF_Null;
			else
			{
				Element->Value = TVal;
				if ( bStringToken )
					Element->Flags |= PEF_StringQuoted;
			}
			return true;
		}
		Cdebugf( "FJSONParser::ParseValue -> Couldn't parse value for %s [%s]", Element->Key.c_str(), Element->Value.c_str() );
	}
	return false;
}

bool CJSONParser::ParseObject( CParserElement* Element)
{
	AdvanceThrough( Data, JSON_SKIP);
	Element->Flags = PEF_Object;
	if ( *Data++ == '{' ) //Needed in case we're parsing a master container (MLUser)
	{
		uint32 PropCount = 0;
		do
		{
			AdvanceThrough( Data, JSON_SKIP);
			if ( *Data == '}' )
			{
				Data++;
				return true;
			}
			if ( *Data == '{' ) //Handle sub-scheme? Is this even valid?
			{}

			if ( (PropCount++ != 0) && (*Data++ != ',') )
			{
				Cdebugf( "FJSONParser::ParseObject -> New child failure in object %s", Element->Key.c_str() );
				TChar8Buffer<32> TmpBuf = Data;
				Cdebugf( "FJSONParser::ParseObject -> Next data in parsing line is [%s ...]", *TmpBuf);
				break;
			}

		} while ( ParseKey(Element) );
	}
	return false;
}

bool CJSONParser::ParseArray( CParserElement* Container)
{
	AdvanceThrough( Data, JSON_SKIP);
	Container->Flags = PEF_Array;

	if ( *Data++ == '[' ) //Needed in case we're parsing a master array (MLCurrency)
	{
		uint32 ArrayNum = 0;
		CParserElement* LastChild = nullptr;
		CParserElement** InsertChildRef = &Container->Children;
		do
		{
			AdvanceThrough( Data, JSON_SKIP);
			if ( *Data == ']' )
			{
				Data++;
				return true;
			}

			if ( (ArrayNum++ != 0) && (*Data++ != ',') )
			{
				Cdebugf( "FJSONParser::ParseArray -> New child failure in array variable %s", Container->Key.c_str() );
				break;
			}

			InsertChildRef[0] = LastChild = new CParserElement( Container->Flags & PEF_Inherit); //Preserve order of array elements
			InsertChildRef = &LastChild->Next;
		} while ( ParseValue(LastChild) );
	}
	return false;
}


//*******************************************************************
// PLAIN TEXT WEB FORM 

bool CPlainTextFormParser::Parse()
{
	while ( (Line=ExtractLine(Data)) != nullptr )
		ParseMembers( &RootElement);
//	RootElement.LogTokens(); //THIS CAN EXPOSE PASSWORDS
	return true;
}

bool CPlainTextFormParser::ParseMembers( CParserElement* Parent)
{
	char* Key = ExtractToken( Line); //Dots are caught by this!!!

	while ( Key && *Key ) //As long as we have a good buffer
	{
		char* Splitter = CStrchr(Key,'.');
		if ( Splitter )
			*Splitter = 0;
		if ( !(CGetCharType(*Key)&CHTYPE_AlNum) && (*Key != '_') )
			return false;
		Parent->Flags |= PEF_Object;
		Parent = Parent->GetChild(Key,true);
		Key = Splitter ? (Splitter+1) : nullptr;
	}
	if ( Parent == &RootElement ) //Failed to traverse from root node (this enables [][] multi array)
		return false;

	if ( *Line == '[' ) //Array
		return ParseArray( Parent);
	if ( *Line == '=' ) //End
	{
		Parent->Value = ++Line;
		return true;
	}
	return false;
}


bool CPlainTextFormParser::ParseArray( CParserElement* Container)
{
	if ( *Line == '[' )
	{
		Line++;
		Container->Flags |= PEF_Array;
		char* ArrayKey = ExtractToken( Line);
		for ( ; *Line==' ' ; Line++ ); //Space skipper
		if ( *Line++ == ']' )
		{
			CParserElement* SelectedNode = nullptr;
			if ( IsNumeric(ArrayKey) )
			{
				uint32 Slot = atoi(ArrayKey);
				if ( Slot < 30 ) //Safety lock
					SelectedNode = Container->GetChild( Slot, true );
			}
			else if ( *ArrayKey == '\0' )  //  [] - consider as 'last'
			{
				if ( Container->GetChild(30) == nullptr ) //No more than 30
					SelectedNode = Container->AddChildLast( new CParserElement(Container->Flags&PEF_Inherit) );
			}

			if ( SelectedNode )
			{
				for ( ; *Line==' ' ; Line++ ); //Space skipper
				if ( *Line == '=' )
				{
					SelectedNode->Value = ++Line;
					return true;
				}
				return ParseMembers( SelectedNode);
			}
		}
	}
	return false;
}
#endif

//*******************************************************************
// PROPERTY PARSERS


#ifdef _VECTOR_
//Helpers - to make life easier
size_t PropertyStdVectorBase::_CalcVectorSize( const CParserElement& Elem, CParserElement*& Child) const
{
	size_t i = 0;
	for ( CParserElement* Link=Elem.Children ; Link ; Link=Link->Next )
		i++;
	Child = Elem.Children;
	return i;
}

void PropertyStdVectorBase::_ImportNextChild( void* Into, CParserElement*& Child) const
{
	if ( Child && Into )
	{
		Inner->Import( Into, *Child);
		Child = Child->Next;
	}
}
#endif

//Importers - move data from elements into properties
void CProperty::Import( void* Into, const CParserElement& Elem) const
{
#ifdef _STRING_
	if ( !Parse( Into, Elem.Value.c_str() ) )
		Cdebugf("Import failed for element [%s] with value %s", Elem.Key, Elem.Value);
#endif
}

void PropertyFixedArray::Import( void* Into, const CParserElement& Elem) const
{
#ifdef _STRING_
	CFixedArray& Array = GetProp<Primitive>(Into);
	if ( !Elem.Children )
	{
		Array.Setup( 0, 0);
		return;
	}
	Array.Setup( Elem.Children->CountTokens(), Inner->ElementSize);
	uint8* Address = (uint8*)Array.GetData();
	for ( CParserElement* Link=Elem.Children ; Link ; Link=Link->Next )
	{
		Inner->Import( Address, *Link);
		Address += Inner->ElementSize;
	}
#endif
}

#ifdef _VECTOR_
void PropertyMasterObjectArray::Import( void* Into, const CParserElement& Elem) const
{
#ifdef _STRING_
	auto& Array = GetProp<Primitive>(Into);
	Array.Empty(); //Destruction handled here
	for ( CParserElement* Link=Elem.Children ; Link ; Link=Link->Next )
	{
		void* NewObject = (*Inner->Model->DefaultCreator)();
		//TODO: ADD SPECIALIZED CLASS CREATOR
		Inner->Import( &NewObject, *Link);
		Array.Add( NewObject);
	}
#endif
}
#endif

void PropertyStruct::Import( void* Into, const CParserElement& Elem) const
{
#ifdef _STRING_
	void* Address = AddressOffset( Into, Offset);
	Elem.ParseObject( Model, Address);
#endif
}

void PropertyStructPtr::Import( void* Into, const CParserElement& Elem) const
{
#ifdef _STRING_
	void** Address = AddressOffset<void*>( Into, Offset);
	Elem.ParseObject( Model, *Address);
#endif
}


//Exporters - create sub-elements using property data (root element must exist)
void CProperty::Export( void* From, CParserElement& Elem) const
{
#ifdef _STRING_
	Elem.Key = Name;
	Elem.Value = String(From);
	if ( IsText() )
		Elem.Flags |= PEF_StringQuoted;
	if ( PropertyFlags & PF_NullDefault )
		Elem.Flags |= PEF_Null;
	if ( PropertyFlags & PF_Inner )
		Elem.Flags |= PEF_Inner;
#endif
}

void PropertyFixedArray::Export( void* From, CParserElement& Elem) const
{
#ifdef _STRING_
	Elem.Flags |= PEF_Array;
	CFixedArray& Array = GetProp<Primitive>(From);
	int_p i = (int_p)Array.Size() - 1;
	uint8* Address = AddressOffset<uint8>( Array.GetData(), (int_p)Inner->ElementSize * i);
	for ( ; i>=0 ; i-- ) //Backwards loop to keep Array order
	{
		CParserElement* Child = new CParserElement( Elem);
		Inner->Export( Address, *Child);
		Address -= Inner->ElementSize;
	}
#endif
}

#ifdef _VECTOR_
void PropertyMasterObjectArray::Export( void* From, CParserElement& Elem) const
{
#ifdef _STRING_
	Elem.Flags |= PEF_Array;
	auto& Array = GetProp<Primitive>(From);
	CSleepLock(&Array.Lock,0);
	for ( int_p i=(int_p)Array.List.size() - 1 ; i>=0 ; i-- ) //Backwards loop to keep Array order
	{
		CParserElement* Child = new CParserElement( Elem);
		Inner->Export( &Array.List[i], *Child);
	}
#endif
}
#endif

void PropertyStruct::Export( void* From, CParserElement& Elem) const
{
#ifdef _STRING_
	Elem.Flags |= PEF_Object;
	void* Address = AddressOffset( From, Offset);
	Elem.ExportObject( Model, Address);
#endif
}

void PropertyStructPtr::Export( void* From, CParserElement& Elem) const
{
#ifdef _STRING_
	Elem.Flags |= PEF_Object;
	void** Address = AddressOffset<void*>( From, Offset);
	Elem.ExportObject( Model, *Address);
#endif
}

