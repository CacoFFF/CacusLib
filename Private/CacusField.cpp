
#include "CacusField.h"
#include "TimeStamp.h"
#include "StackUnwinder.h"

#include <map>


// Struct list
struct cmp_str
{
	bool operator()(char const *a, char const *b) const
	{
		return strcmp(a, b) < 0; //Use CStrcmp
	}
};
std::map<const char*,CStruct*,cmp_str> StructMap;

CStruct* GetStruct( const char* StructName)
{
	auto Entry = StructMap.find( StructName);
	return (Entry != StructMap.end()) ? Entry->second : nullptr;
}




CField::CField( const char* InName, CStruct* InParent)
	: Name(InName)
	, Parent( InParent)
{
	if ( InParent )
	{
		Next = InParent->Children;
		InParent->Children = this;
	}
	else
		Next = nullptr;
}

bool CField::IsType( const char* Type) const
{
	return !_stricmp( Type, TypeName() );
}

CStruct::CStruct( const char* InName, CStruct* InSuper, STRUCT_CREATOR InDefaultCreator, STRUCT_DESTRUCTOR InDefaultDestructor)
	: CField(InName)
	, DefaultCreator(InDefaultCreator)
	, DefaultDestructor(InDefaultDestructor)
	, PostParseFunction(nullptr)
	, PropertiesSize(0)
	, SuperStruct(InSuper)
	, Children(nullptr)
	, Properties(nullptr)
	, DestructorLink(nullptr)
	, DefaultObject( (*InDefaultCreator)() )
{
	StructMap.emplace( InName, this);
}

CField* CStruct::FindField( const char* FieldName) const
{
	if ( FieldName )
	{
		for ( CField* Link=Children ; Link ; Link=Link->Next )
			if ( _stricmp( Link->Name, FieldName) )
				return Link;
		if ( SuperStruct )
			return SuperStruct->FindField( FieldName);
	}
	return nullptr;
}

CProperty* CStruct::FindProperty( const char* PropertyName) const
{
	guard(CStruct::FindProperty);
	if ( PropertyName )
	{
		if ( !PropertyName[0] ) //Find a default property instead
			PropertyName = "DefaultProperty";
		for ( CProperty* Link=Properties ; Link ; Link=Link->NextProperty )
			if ( !_stricmp( Link->Name, PropertyName) )
				return Link;
		if ( SuperStruct )
			return SuperStruct->FindProperty( PropertyName);
	}
	return nullptr;
	unguard;
}

void CStruct::DestroyProperties( void* Object)
{
	for ( CProperty* Link=DestructorLink ; Link ; Link=Link->NextDestructor )
		Link->DestroyValue( Object);
	if ( SuperStruct )
		SuperStruct->DestroyProperties( Object);
}

void CStruct::GenericDescribe( void* Object) const
{
	if ( SuperStruct )
		SuperStruct->GenericDescribe( Object);
	for ( CProperty* Link=Properties ; Link ; Link=Link->NextProperty )
		printf( "%s = %s \n", Link->Name, Link->String(Object) );
}

CProperty::CProperty( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InElementSize, size_t InOffset, uint32 InPropertyFlags)
	: CField( InName, InParent)
	, ArrayDim( InArrayDim)
	, ElementSize( InElementSize)
	, Offset( InOffset)
	, PropertyFlags( InPropertyFlags)
	, NextProperty( InParent ? InParent->Properties : nullptr)
	, NextDestructor( nullptr)
{
//	printf( "Created property %s.%s[%i] at=%i[%i], flags=%i \n", InParent ? *InParent->Name : "::", InName, ArrayDim, Offset, ElementSize, PropertyFlags);
	if ( InParent )
	{
		Parent->Properties = this;
		int32 EndOffset = Offset + ElementSize * ArrayDim;
		if ( EndOffset > Parent->PropertiesSize )
			Parent->PropertiesSize = EndOffset;
		if ( PropertyFlags | PF_Destructible )
		{
			NextDestructor = Parent->DestructorLink;
			Parent->DestructorLink = this;
		}
	}
}

//=======================
// PropertyStdString

bool PropertyStdString::Parse( void* Into, const char* From) const
{
	Primitive& Prop = GetProp<Primitive>(Into);
	Prop = From;
	return true;
}

bool PropertyStdString::Booleanize( void* Object) const
{
	Primitive& Prop = GetProp<Primitive>(Object);
	return Prop.length() > 0;
}

void PropertyStdString::DestroyValue( void* Object) const
{
	Primitive& Prop = GetProp<Primitive>(Object);
	Prop.~basic_string();
}

const char* PropertyStdString::String( void* Object) const
{
	return CopyToBuffer( GetProp<Primitive>(Object).c_str() );
}


//=======================
// PropertyC8Text

bool PropertyC8Text::Parse( void* Into, const char* From) const
{
	char* Prop = (char*)Into + Offset;
	if ( PropertyFlags & PF_UTF8Parse )
	{
		//		utf8::Decode( Prop, BufSize, From);
		return true;
	}
	else
		return !CStrcpy_s( Prop, BufSize, From);
}

bool PropertyC8Text::Booleanize( void* Object) const
{
	char* Prop = (char*)Object + Offset;
	return *Prop != 0;
}

const char* PropertyC8Text::String( void* Object) const //SHEEEEEEEIT
{
	if ( PropertyFlags & PF_UTF8Parse )
	{
		//TODO: Convert to UTF-8 encoded string
	}
	return CopyToBuffer( (char*)Object + Offset, BufSize );
}

//=======================
// PropertyC16Text


bool PropertyC16Text::Parse( void* Into, const char* From) const
{
	char16_t* Buffer = (char16_t*)((uint8*)Into + Offset);
	if ( PropertyFlags & PF_UTF8Parse )
	{
		utf8::Decode( Buffer, BufSize, From);
		return true;
	}
	else
		return !CStrcpy_s( Buffer, BufSize, From);
}

bool PropertyC16Text::Booleanize( void* Object) const
{
	char16_t* Buffer = (char16_t*)((uint8*)Object + Offset);
	return *Buffer != 0;
}

const char* PropertyC16Text::String( void* Object) const //SHEEEEEEEIT
{
	char16_t* SrcBuffer = (char16_t*)((uint8*)Object + Offset);
	if ( PropertyFlags & PF_UTF8Parse )
	{
		auto Len = utf8::EncodedLen(SrcBuffer);
		char* DestBuffer = CharBuffer<char>( Len + 1);
		utf8::Encode( DestBuffer, Len+1, SrcBuffer);
		return DestBuffer;
	}
	return "";
}


//=======================
// PropertyFloat

bool PropertyFloat::Parse( void* Into, const char* From) const
{
	Primitive& Prop = GetProp<Primitive>(Into);
	Prop = (float)atof(From);
	return true;
}

bool PropertyFloat::Booleanize( void* Object) const
{
	int32& Prop = *(int32*)((uint8*)Object + Offset); //Booleanize using Integer
	return Prop != 0;
}

const char* PropertyFloat::String( void* Object) const
{
	Primitive& Prop = GetProp<Primitive>(Object);
	return CSprintf( "%f", Prop);
}

//=======================
// PropertyInt32

bool PropertyInt32::Parse( void* Into, const char* From) const
{
	Primitive& Prop = GetProp<Primitive>(Into);
	Prop = atoi(From);
	return true;
}

bool PropertyInt32::Booleanize( void* Object) const
{
	Primitive& Prop = GetProp<Primitive>(Object);
	return Prop != 0;
}

const char* PropertyInt32::String( void* Object) const
{
	Primitive& Prop = GetProp<Primitive>(Object);
	return CSprintf( "%i", Prop);
}

//=======================
// PropertyBool

bool PropertyBool::Parse( void* Into, const char* From) const
{
	Primitive& Prop = GetProp<Primitive>(Into);
	if ( !_stricmp( From, "true") || !_stricmp( From, "1") )
	{
		Prop = true;
		return true;
	}
	else if ( !_stricmp( From, "false") || !_stricmp( From, "0") )
	{
		Prop = false;
		return true;
	}
	else
		return false;
}

bool PropertyBool::Booleanize( void* Object) const
{
	return GetProp<Primitive>(Object);
}

const char* PropertyBool::String( void* Object) const
{
	return GetProp<Primitive>(Object) ? "true" : "false";
}


//=======================
// PropertyEnum

bool PropertyEnum::Parse( void* Into, const char* From) const
{
	for ( uint32 i=0 ; i<EnumCount ; i++ )
		if ( !_stricmp(EnumLiterals[i], From) )
		{
			uint32& Prop = *(uint32*)((uint8*)Into + Offset);
			Prop = i;
			return true;
		}
	return false;
}

bool PropertyEnum::Booleanize( void* Object) const
{
	return true;
}

const char* PropertyEnum::String( void* Object) const
{
	int32& Prop = *(int32*)((uint8*)Object + Offset);
	if ( Prop >= 0 && (uint32)Prop < EnumCount )
		return EnumLiterals[Prop];
	return "";
}

//=======================
// PropertyFixedArray

bool PropertyFixedArray::Booleanize( void* Object) const
{
	return GetProp<Primitive>(Object).Size() > 0;
}

void PropertyFixedArray::DestroyValue( void* Object) const
{
	GetProp<Primitive>(Object).~CFixedArray();
}

//=======================
// PropertyMasterObjectArray

bool PropertyMasterObjectArray::Booleanize( void* Object) const
{
	return GetProp<Primitive>(Object).List.size() > 0;
}

void PropertyMasterObjectArray::DestroyValue( void* Object) const
{
	GetProp<Primitive>(Object).Empty();
	GetProp<Primitive>(Object).~TMasterObjectArray();
}

//=======================
// PropertyStdVectorBase


//=======================
// PropertyStruct

bool PropertyStruct::Booleanize( void* Object) const //If any sub-property booleanizes, this booleanizes as well
{
	void* InnerStruct = (void*)((uint8*)Object + Offset);
	for ( CProperty* Link=(Model?Model->Properties:nullptr) ; Link ; Link=Link->NextProperty )
		if ( Link->Booleanize( InnerStruct) )
			return true;
	return false;
}

//=======================
// PropertyStructPtr

bool PropertyStructPtr::Booleanize( void* Object) const
{
	void*& InnerStructPtr = GetProp<void*>(Object);
	if ( InnerStructPtr )
	{
		for ( CProperty* Link=(Model?Model->Properties:nullptr) ; Link ; Link=Link->NextProperty )
			if ( Link->Booleanize( InnerStructPtr) )
				return true;
	}
	return false;
}

void PropertyStructPtr::DestroyValue( void * Object) const
{
	if ( PropertyFlags | PF_Destructible ) //Objects references by this need to be destroyed
	{
		void*& InnerStructPtr = GetProp<void*>(Object);
		(*Model->DefaultDestructor)(InnerStructPtr);
	}
}


//=======================
// PropertyTimestampDateTime

bool PropertyTimestampDateTime::Parse( void* Into, const char* From) const
{
	Primitive& Prop = GetProp<Primitive>(Into);
	Prop = FTimestampDateTime::Parse( From);
	return Prop; //Booleanize (if non-zero, parse was succesful)
}

bool PropertyTimestampDateTime::Booleanize( void* Object) const
{
	return GetProp<Primitive>(Object);
}

const char* PropertyTimestampDateTime::String( void* Object) const
{
	return CopyToBuffer( **GetProp<Primitive>(Object) );
}

