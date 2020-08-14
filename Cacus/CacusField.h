/*=============================================================================
	CacusField.h
	Author: Fernando Velázquez

	Class/Struct and Property descriptor system.
	Allows high level sybsystems to operate on low level objects and primitives.
=============================================================================*/



#ifndef USES_CACUS_FIELD
#define USES_CACUS_FIELD

#include "Atomics.h"
#include "CacusTemplate.h"
#include "TCharBuffer.h"

class CStruct;

/*------
	These macros facilitate the creation of a CStruct descriptor for a class.

	The name of the struct(s) should always be represented by their full name
	which includes namespaces and containing structs/classes.

	If your class is an element of an array property (std::vector or TFixedArray)
	it needs to implement the following comparison operators:
bool operator==( const thisclass& Other) const;
bool operator!=( const thisclass& Other) const;
------*/

#define CSTRUCT_DECLARE_BASE_CLASS(thisclass) \
public: \
	typedef thisclass StructPrimitive; \
	static const char* StaticStructName() { return #thisclass; } \
	static CStruct* GetInstanceCStruct(); \
	static void CStructInit( CStruct* st_Struct);

#define CSTRUCT_DECLARE_CLASS(thisclass,superclass) \
public: \
	CSTRUCT_DECLARE_BASE_CLASS(thisclass) \
	typedef superclass Super; \

#define CSTRUCT_VIRTUAL_CLASS \
	virtual CStruct* VirtualCStruct() { return GetInstanceCStruct(); }

#define CSTRUCT_NODELTA_COMPARE \
	bool operator==( const StructPrimitive& Other) const { return false; } \
	bool operator!=( const StructPrimitive& Other) const { return true; }

/*------
	These macros facilitate the implementation of a CStruct descriptor for a class.

	Use CSTRUCT_IMPLEMENT_BASE_CLASS or CSTRUCT_IMPLEMENT_CLASS and then define
------*/

#define CSTRUCT_IMPLEMENT_BASE_CLASS(thisclass) \
	CStruct* thisclass::GetInstanceCStruct() \
	{ 	static CStructCreator<thisclass> InstanceCStruct; return &InstanceCStruct;	}

#define CSTRUCT_IMPLEMENT_CLASS(thisclass,superclass) \
	CStruct* thisclass::GetInstanceCStruct() \
	{ 	static CStructCreator<thisclass> InstanceCStruct(superclass::GetInstanceCStruct()); return &InstanceCStruct;	}


/*------
	Specialize this function with your class as argument and use the macros to create
	properties and descriptors as well as modifying the new CStruct object.

void Class::CStructInit( CStruct* st_Struct)
{
		// Creating property descriptors:
	CREATE_PROPERTY( PROPERTYNAME_1)
	CREATE_PROPERTY( PROPERTYNAME_2, optional_flags) //See from EPropertyFlags enum.

		// Creating class descriptor (CSTRUCT_VIRTUAL_CLASS required in class definition)
		// Creates a field named 'Class' which helps importers figure out which objects type to create.
	CREATE_CLASS_MARKER() 
}
------*/

#define CREATE_PROPERTY(propname,...) \
		CreateProperty( #propname, st_Struct, CSTRUCT_MEMBER(StructPrimitive,propname),##__VA_ARGS__);

#define CREATE_FUNCTION(propname,...) \
		CreateFunction( #propname, st_Struct, ##__VA_ARGS__);

#define CREATE_CLASS_MARKER() \
		CreateFunction( "Class", st_Struct, &GetCStructClassName<StructPrimitive> )->PropertyFlags &= ~PF_NoExport; 
//		CreateFunction( "Class", st_Struct, [](void* Obj){ if (Obj) return ((StructPrimitive*)Obj)->VirtualCStruct()->Name; return StructPrimitive::GetInstanceCStruct()->Name;})->PropertyFlags &= ~PF_NoExport; 

#define CREATE_DEFAULT_PROPERTY(propname,...) \
		CREATE_PROPERTY(propname,##__VA_ARGS__) \
		CreateProperty( "DefaultProperty", st_Struct, CSTRUCT_MEMBER(StructPrimitive,propname),##__VA_ARGS__);

template <class T> const char* GetCStructClassName( void* Obj)
{
	if ( (T*)Obj )
		return ((T*)Obj)->VirtualCStruct()->Name;
	return T::GetInstanceCStruct()->Name;
}


//=============================================================================
//================ CFIELD SYSTEM
//=============================================================================

#define DECLARE_FIELD(classname,superclass) \
	typedef superclass Super; \
	typedef classname ThisType; \
	public: \
	virtual const char* TypeName() const { return #classname; } \
	virtual bool IsTypeA( const char* Type) const { if (IsType(Type) ) return true; return Super::IsTypeA(Type); }

#define SET_PRIMITIVE(primitive) \
	typedef primitive Primitive; \
	Primitive& GetValue( void* From) const { return GetProp<Primitive>(From); } \
	virtual bool ShouldExport( void* From, void* Delta) const \
	{	return Super::ShouldExport(From,Delta) && (!Delta || (PropertyFlags&PF_NoDeltaExport) || GetValue(From)!=GetValue(Delta));	}


#define LAMBDA_CREATOR(structname) [](){ return (void*)new structname(); }
#define LAMBDA_DESTRUCTOR(structname) [](void*& Obj){ if (Obj) delete (structname*)Obj; Obj=nullptr; }



enum EParseType
{
	PARSE_PlainTextForm, //method="POST" enctype="text/plain"
	PARSE_Ini,
	PARSE_JSON,
	PARSE_MAX,
};

enum EPropertyFlags
{
	PF_NoConfig             = 0x00000001,
	PF_NoSerialize          = 0x00000002,

	PF_Destructible         = 0x00000008,
	PF_NullDefault          = 0x00000010, //Interpret default value as null in JSON
	PF_UTF8Parse            = 0x00000020,
	PF_NoExport             = 0x00000040,
	PF_NoImport             = 0x00000080,
	PF_NoDeltaExport        = 0x00000100, //Always export, even if delta

	PF_Inner                = 0x00000200,

	PF_Deprecated           = PF_NoExport | PF_NoImport | PF_NoConfig,
};

template<typename T> constexpr bool IsEnumType()           { return false; }


template<typename T> constexpr bool IsBasicType()          { return false; }
template<> constexpr bool IsBasicType<int8>()              { return true; }
template<> constexpr bool IsBasicType<int16>()             { return true; }
template<> constexpr bool IsBasicType<int32>()             { return true; }
template<> constexpr bool IsBasicType<int64>()             { return true; }
template<> constexpr bool IsBasicType<uint8>()             { return true; }
template<> constexpr bool IsBasicType<uint16>()            { return true; }
template<> constexpr bool IsBasicType<uint32>()            { return true; }
template<> constexpr bool IsBasicType<uint64>()            { return true; }
template<> constexpr bool IsBasicType<float>()             { return true; }
template<> constexpr bool IsBasicType<bool>()              { return true; }

template<typename T> constexpr bool IsStructType()         { return !IsEnumType<T>() && !IsBasicType<T>(); } //Heuristics based

class CParserElement;
class CField;
class CStruct;
class CProperty;
class PropertyInt32;
class PropertyFloat;
class PropertyBool;

extern "C" CACUS_API CStruct* GetStruct( const char* StructName);
extern "C" CACUS_API bool DataToObject( const char* Data, void* Into, CProperty* Outer, EParseType ImportType);
extern "C" CACUS_API void ObjectToObject( void* From, void* Into, CProperty* FromOuter, CProperty* IntoOuter);
#ifdef _STRING_
extern "C" CACUS_API void ObjectToData( std::string& OutData, void* From, CProperty* Outer, EParseType ExportType, int DeltaDefaults=0);
extern "C" CACUS_API bool DataToData( const char* InData, std::string& OutData, EParseType InFormat, EParseType OutFormat);
#endif

class CACUS_API CField
{
public:
	const char* Name;
	CStruct* Parent;
	CField* Next;

	CField( const char* InName, CStruct* InParent=nullptr);

	bool IsType( const char* Type) const;

	virtual const char* TypeName() const
	{
		return "";
	}

	virtual bool IsTypeA( const char* Type) const
	{
		return IsType(Type);
	}

};


typedef void* (*STRUCT_CREATOR)();
typedef void (*STRUCT_DESTRUCTOR)(void*&);
typedef void (*STRUCT_POST_PARSE)(void*);
class CACUS_API CStruct : public CField
{
	DECLARE_FIELD(CStruct,CField)

	STRUCT_CREATOR DefaultCreator;
	STRUCT_DESTRUCTOR DefaultDestructor;
	STRUCT_POST_PARSE PostParseFunction;
	int32 PropertiesSize;
	CStruct* SuperStruct;
	CField* Children;
	CProperty* Properties;
	CProperty* DestructorLink;
	void* DefaultObject;

	CStruct( const char* InName, CStruct* InSuper, STRUCT_CREATOR InDefaultCreator, STRUCT_DESTRUCTOR DefaultDestructor);

	CField* FindField( const char* FieldName) const;
	CProperty* FindProperty( const char* PropName) const;

	void DestroyProperties( void* Object);

	void GenericDescribe( void* Object) const;
};

template <class T> class CStructCreator : public CStruct
{
public:
	CStructCreator( CStruct* InSuper=nullptr)
		: CStruct( T::StaticStructName(), InSuper, LAMBDA_CREATOR(T), LAMBDA_DESTRUCTOR(T) )
	{
		T::CStructInit(this);
	}
};


class CACUS_API CProperty : public CField
{
	DECLARE_FIELD(CProperty,CField)

	int32 ArrayDim;
	size_t ElementSize;
	size_t Offset;
	uint32 PropertyFlags;
	CProperty* NextProperty;
	CProperty* NextDestructor;

	CProperty( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InElementSize, size_t InOffset, uint32 InPropertyFlags);

	friend class CParserElement;
	static bool CParserLog;

	virtual bool Parse( void* Into, const char* From) const              { return false; }
	virtual void Import( void* Into, const CParserElement& Elem) const;
	virtual void Export( void* From, CParserElement& Elem) const;
	virtual bool ShouldExport( void* From, void* Delta=nullptr) const    { return !(PropertyFlags & PF_NoExport); }
	virtual bool Booleanize( void* Object) const = 0;
	virtual bool IsPointer() const                                       { return false; }
	virtual bool IsText() const                                          { return false; }
	virtual void DestroyValue( void* Object) const                       { return; };
	virtual const char* String( void* Object) const                      { return ""; };
	virtual CProperty* GetInner() const                                  { return nullptr; }

	template<typename T> T& GetProp( void* Obj) const                    { return *(T*)((uint8*)Obj + Offset); }
	template<typename T> T* GetAddr( void* Obj) const                    { return (T*)((uint8*)Obj + Offset); }
};


//=============================================================================
//================ PROPERTY DESCRIPTORS
//=============================================================================


#ifdef _STRING_
class CACUS_API PropertyStdString : public CProperty
{
	DECLARE_FIELD(PropertyStdString,CProperty)
	SET_PRIMITIVE(std::string)

	PropertyStdString( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: CProperty( InName, InParent, InArrayDim, sizeof(Primitive), InOffset, InPropertyFlags | PF_Destructible)	{}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	bool IsText() const                                                  { return true; }
	void DestroyValue( void* Object) const;
	const char* String( void* Object) const;
};
#endif

class CACUS_API PropertyC8TextBase : public CProperty
{
	DECLARE_FIELD(PropertyC8TextBase,CProperty)
	size_t BufSize;

	PropertyC8TextBase( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, size_t InBufSize)
		: CProperty( InName, InParent, InArrayDim, sizeof(char)*InBufSize, InOffset, InPropertyFlags)
		, BufSize( InBufSize)
	{
		if ( InPropertyFlags & PF_UTF8Parse )
			throw "PropertyC8Text should not be used with UTF8 parsing";
		if ( InBufSize == 0 )
			throw "PropertyC8Text with zero length";
	}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	bool IsText() const                                                  { return true; }
	const char* String( void* Object) const;

	//Custom export rule
	bool ShouldExport( void* From, void* Delta) const
	{
		return Super::ShouldExport(From,Delta) && (!Delta || (PropertyFlags&PF_NoDeltaExport) || CStrncmp(GetAddr<char>(From),GetAddr<char>(Delta),BufSize));
	}
};

class CACUS_API PropertyC16TextBase : public CProperty
{
	DECLARE_FIELD(PropertyC16TextBase,CProperty)
	size_t BufSize;

	PropertyC16TextBase( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, size_t InBufSize)
		: CProperty( InName, InParent, InArrayDim, sizeof(char16_t)*InBufSize, InOffset, InPropertyFlags)
		, BufSize( InBufSize) {}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	bool IsText() const                                                  { return true; }
	const char* String( void* Object) const;

	//Custom export rule
	bool ShouldExport( void* From, void* Delta) const
	{
		return Super::ShouldExport(From,Delta) && (!Delta || (PropertyFlags&PF_NoDeltaExport) || CStrncmp(GetAddr<char16>(From),GetAddr<char16>(Delta),BufSize));
	}
};

template <size_t TSize> struct PropertyC8Text : public PropertyC8TextBase
{
	PropertyC8Text( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: PropertyC8TextBase( InName, InParent, InArrayDim, InOffset, InPropertyFlags, TSize)
	{}
};
template <size_t TSize> struct PropertyC16Text : public PropertyC16TextBase
{
	PropertyC16Text( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: PropertyC16TextBase( InName, InParent, InArrayDim, InOffset, InPropertyFlags, TSize)
	{}
};

class CACUS_API PropertyInt32 : public CProperty
{
	DECLARE_FIELD(PropertyInt32,CProperty)
	SET_PRIMITIVE(int32)

	PropertyInt32( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: CProperty( InName, InParent, InArrayDim, sizeof(int32), InOffset, InPropertyFlags)	{}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	const char* String( void* Object) const;
};

class CACUS_API PropertyUInt32 : public CProperty
{
	DECLARE_FIELD(PropertyUInt32,CProperty)
	SET_PRIMITIVE(uint32)

	PropertyUInt32( const char* InName, CStruct* InParent, uint32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: CProperty( InName, InParent, InArrayDim, sizeof(uint32), InOffset, InPropertyFlags)	{}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	const char* String( void* Object) const;
};

class CACUS_API PropertyFloat : public CProperty
{
	DECLARE_FIELD(PropertyFloat,CProperty)
	SET_PRIMITIVE(float)

	PropertyFloat( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: CProperty( InName, InParent, InArrayDim, sizeof(float), InOffset, InPropertyFlags)	{}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	const char* String( void* Object) const;
};

class CACUS_API PropertyBool : public CProperty
{
	DECLARE_FIELD(PropertyBool,CProperty)
	SET_PRIMITIVE(bool)

	PropertyBool( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: CProperty( InName, InParent, InArrayDim, sizeof(bool), InOffset, InPropertyFlags)	{}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	const char* String( void* Object) const;
};

class CACUS_API PropertyEnum : public CProperty
{
	DECLARE_FIELD(PropertyEnum,CProperty)
	SET_PRIMITIVE(int)

	const char** EnumLiterals;
	uint32 EnumCount;

	template <uint32 InCount> PropertyEnum( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, const char* (&InLiterals)[InCount] )
		: CProperty( InName, InParent, InArrayDim, sizeof(int), InOffset, InPropertyFlags)
		, EnumLiterals(InLiterals)
		, EnumCount(InCount)	{}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	bool IsText() const                                                  { return true; }
	const char* String( void* Object) const;

	template<typename T,uint32 InCount> friend PropertyEnum* CreateProperty( const char* Name, CStruct* Parent, T& Prop, const char* (&InLiterals)[InCount], uint32 PropertyFlags=0 )
	{
		static_assert( sizeof(T) == sizeof(int), "NOT AN ENUM(int32) PROPERTY");
		return new PropertyEnum( Name, Parent, 1, ((size_t)&Prop) - CSTRUCT_BASE, PropertyFlags, InLiterals);
	}
};

class CACUS_API PropertyFixedArray : public CProperty
{
	DECLARE_FIELD(PropertyFixedArray,CProperty)
	SET_PRIMITIVE(CFixedArray)

	CProperty* Inner;

	PropertyFixedArray( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, CProperty* InInner)
		: CProperty( InName, InParent, InArrayDim, sizeof(Primitive), InOffset, InPropertyFlags | PF_Destructible)
		, Inner( InInner)
	{	Inner->Offset = 0; /*Important*/	}

	void Import( void* Into, const CParserElement& Elem) const;
	void Export( void* From, CParserElement& Elem) const;
	bool Booleanize( void* Object) const;
	void DestroyValue( void* Object) const;
	CProperty* GetInner() const                                          { return Inner; }
};

class CACUS_API PropertyStruct : public CProperty
{
	DECLARE_FIELD(PropertyStruct,CProperty)

	CStruct* Model;

	PropertyStruct( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, CStruct* InModel)
		: CProperty( InName, InParent, InArrayDim, InModel->PropertiesSize, InOffset, InPropertyFlags)
		, Model( InModel)	{}

	void Import( void* Into, const CParserElement& Elem) const;
	void Export( void* From, CParserElement& Elem) const;
	bool Booleanize( void* Object) const;
};

class CACUS_API PropertyStructPtr : public CProperty
{
	DECLARE_FIELD(PropertyStructPtr,CProperty)

	CStruct* Model;

	PropertyStructPtr( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, CStruct* InModel)
		: CProperty( InName, InParent, InArrayDim, sizeof(void*), InOffset, InPropertyFlags)
		, Model( InModel)	{}

	void Import( void* Into, const CParserElement& Elem) const;
	void Export( void* From, CParserElement& Elem) const;
	bool Booleanize( void* Object) const;
	void DestroyValue( void* Object) const;
};

#ifdef _VECTOR_
class CACUS_API PropertyMasterObjectArray : public CProperty
{
	DECLARE_FIELD(PropertyMasterObjectArray,CProperty)
	SET_PRIMITIVE(TMasterObjectArray<void*>)

	PropertyStructPtr* Inner; //Locked to PropertyStruct

	PropertyMasterObjectArray( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, PropertyStructPtr* InInner)
		: CProperty( InName, InParent, InArrayDim, sizeof(void*), InOffset, InPropertyFlags | PF_Destructible)
		, Inner( InInner)
	{	Inner->Offset = 0; /*Important*/	}

	void Import( void* Into, const CParserElement& Elem) const;
	void Export( void* From, CParserElement& Elem) const;
	bool Booleanize( void* Object) const;
	void DestroyValue( void* Object) const;
	CProperty* GetInner() const                                          { return Inner; }
};

class CACUS_API PropertyStdVectorBase : public CProperty
{
	DECLARE_FIELD(PropertyStdVectorBase,CProperty)

	CProperty* Inner;

	PropertyStdVectorBase( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InElementSize, size_t InOffset, uint32 InPropertyFlags, CProperty* InInner)
		: CProperty( InName, InParent, InArrayDim, InElementSize, InOffset, InPropertyFlags | PF_Destructible )
		, Inner( InInner)
	{	Inner->Offset = 0; /*Important*/	}

	void Import( void* Into, const CParserElement& Elem) const;
	void Export( void* From, CParserElement& Elem) const;
	bool Booleanize( void* Object) const;

	CProperty* GetInner() const                                          { return Inner; }
	size_t _CalcVectorSize( const CParserElement& Elem, CParserElement*& Child) const;
	void _ImportNextChild( void* Into, CParserElement*& Child) const;

	virtual size_t VectorGetSize( void* Object) const = 0;
	virtual void   VectorSetSize( void* Object, size_t NewSize) const = 0;
	virtual void*  VectorGetElement( void* Object, size_t Index) const = 0;
};

//WARNING: USAGE OF THIS PROPERTY REQUIRES PRIMITIVE TO HAVE:
// - [==],[!=] OPERATORS
// - Associated CProperty INNER for 'T'
template< class T > class PropertyStdVector : public PropertyStdVectorBase
{
	DECLARE_FIELD(PropertyStdVector,PropertyStdVectorBase)
	SET_PRIMITIVE(std::vector<T>)

	PropertyStdVector( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags, CProperty* InInner)
		: PropertyStdVectorBase( InName, InParent, InArrayDim, sizeof(Primitive), InOffset, InPropertyFlags, InInner)	{}

	void DestroyValue( void* Object) const                               { GetProp<Primitive>(Object).~Primitive(); }
	size_t VectorGetSize( void* Object) const                            { return GetProp<Primitive>(Object).size(); }
	void   VectorSetSize( void* Object, size_t NewSize) const            { return GetProp<Primitive>(Object).resize(NewSize); }
	void*  VectorGetElement( void* Object, size_t Index) const           { return &GetProp<Primitive>(Object)[Index]; }
};

#endif


//Primitive-less property, this calls a function with the object as parameter 1, and additional object as parameter 2
typedef const char* (*PROP_STRING_FUNCTION)(void*);
typedef bool (*PROP_BOOL_FUNCTION)(void*);
class CACUS_API PropertyStringFunction : public CProperty
{
	DECLARE_FIELD(PropertyStringFunction,CProperty)

	PROP_STRING_FUNCTION StrFunction;
	PROP_BOOL_FUNCTION BoolFunction;


	PropertyStringFunction( const char* InName, CStruct* InParent, PROP_STRING_FUNCTION InStr, PROP_BOOL_FUNCTION InBool )
		: CProperty( InName, InParent, 0, 0, 0, PF_NoSerialize|PF_Deprecated)
		, StrFunction(InStr)
		, BoolFunction(InBool) {}

	bool Booleanize( void* Object) const                                 { return (*BoolFunction)(Object); }
	const char* String( void* Object) const                              { return (*StrFunction)(Object); }
	static bool DefaultBooleanize( void*)                                { return false; }
};

inline CProperty* CreateFunction( const char* Name, CStruct* Parent, PROP_STRING_FUNCTION StrFunc, PROP_BOOL_FUNCTION BoolFunc=&PropertyStringFunction::DefaultBooleanize )
{
	return new PropertyStringFunction(Name, Parent, StrFunc, BoolFunc);
}


//=============================================================================
//================ PROPERTY CREATORS
//=============================================================================

#define CP_CREATE(Type,...) \
	new Type(Name,Parent,1,((int_p)&Prop)-CSTRUCT_BASE,PropertyFlags,##__VA_ARGS__);

template<typename T> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, T*& Prop, uint32 PropertyFlags=0)
{
	CStruct* Struct = T::GetInstanceCStruct();
	if ( !Struct->PropertiesSize )
		Struct->PropertiesSize = sizeof(T); //Ensures proper allocation sizes
	return CP_CREATE(PropertyStructPtr, Struct); //Will fail if type has no 'GetInstanceCStruct' function
}

template<typename T> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, T& Prop, uint32 PropertyFlags=0)
{
	// If your compiler brought you here, then you must either:
	// Implement GetInstanceCStruct in your new struct, or if the struct has a property type,
	// include CacusField.h first in your CPP file so that the struct's property type is active.
	CStruct* Struct = T::GetInstanceCStruct();
	if ( !Struct->PropertiesSize )
		Struct->PropertiesSize = sizeof(T); //Ensures proper allocation sizes
	return CP_CREATE(PropertyStruct, Struct); //Will fail if type has no 'GetInstanceCStruct' function
}
template<> inline CProperty* CreateProperty<int32>( const char* Name, CStruct* Parent, int32& Prop, uint32 PropertyFlags)
{
	return CP_CREATE(PropertyInt32);
}
template<> inline CProperty* CreateProperty<uint32>( const char* Name, CStruct* Parent, uint32& Prop, uint32 PropertyFlags)
{
	return CP_CREATE(PropertyUInt32);
}
template<> inline CProperty* CreateProperty<float>( const char* Name, CStruct* Parent, float& Prop, uint32 PropertyFlags)
{
	return CP_CREATE(PropertyFloat);
}
template<> inline CProperty* CreateProperty<bool>( const char* Name, CStruct* Parent, bool& Prop, uint32 PropertyFlags)
{
	return CP_CREATE(PropertyBool);
}
#ifdef _STRING_
template<> inline CProperty* CreateProperty<std::string>( const char* Name, CStruct* Parent, std::string& Prop, uint32 PropertyFlags)
{
	return CP_CREATE(PropertyStdString);
}
#endif


template<typename A> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, TFixedArray<A>& Prop, uint32 PropertyFlags=0)
{
	CProperty* SubProp = CreateProperty( "Inner", nullptr, *((A*)CSTRUCT_BASE), PropertyFlags|PF_Inner);
	return CP_CREATE(PropertyFixedArray, SubProp);
}

#ifdef _VECTOR_
template<typename A> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, TMasterObjectArray<A>& Prop, uint32 PropertyFlags=0)
{
	CProperty* SubProp = CreateProperty( "Inner", nullptr, *((A*)CSTRUCT_BASE), PropertyFlags|PF_Inner);
	if ( SubProp->IsTypeA("PropertyStructPtr") )
		return CP_CREATE(PropertyMasterObjectArray, (PropertyStructPtr*)SubProp);
	throw "CreateProperty: Incorrect property type";
	//	return nullptr;
}

template<class T> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, std::vector<T,std::allocator<T>>& Prop, uint32 PropertyFlags=0)
{
	CProperty* SubProp = CreateProperty( "Inner", nullptr, *((T*)CSTRUCT_BASE), PropertyFlags|PF_Inner);
	return CP_CREATE(PropertyStdVector<T>, SubProp);
}
#endif

template<size_t BufSize> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, TChar8Buffer<BufSize>& Prop, uint32 PropertyFlags=0)
{
	return CP_CREATE(PropertyC8Text<BufSize>);
}

template<size_t ArrayDim> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, char (&Prop)[ArrayDim], uint32 PropertyFlags=0)
{
	return CP_CREATE(PropertyC8Text<ArrayDim>);
}

template<size_t BufSize> inline CProperty* CreateProperty( const char* Name, CStruct* Parent, TChar16Buffer<BufSize>& Prop, uint32 PropertyFlags=0)
{
	return CP_CREATE(PropertyC16Text<BufSize>);
}



//=============================================================================
//================ IMPORTER+EXPORTER HELPERS
//=============================================================================

#define DEFAULTS(type) ((type*)(type::GetInstanceCStruct()->DefaultObject))

template< typename T > void DataToObject( const char* Data, T& Into, EParseType ParseType)
{
	auto* Outer = CreateProperty("",nullptr,*(T*)nullptr,PF_Inner);
	DataToObject( Data, (void*)&Into, Outer, ParseType);
	delete Outer;
}


template< typename T > std::string ObjectToData( const T& From, EParseType ExportType, int DeltaDefaults=0)
{
	auto* Outer = CreateProperty("",nullptr,*(T*)nullptr,PF_Inner);
	std::string Result;
	ObjectToData( Result, (void*)&From, Outer, ExportType, DeltaDefaults);
	delete Outer;
	return Result;
}

template< typename T1, typename T2 > void ObjectToObject( const T1& From, T2& Into)
{
	auto* FromOuter = CreateProperty("",nullptr,*(T1*)nullptr,PF_Inner);
	auto* IntoOuter = CreateProperty("",nullptr,*(T2*)nullptr,PF_Inner);
	ObjectToObject( (void*)&From, (void*)&Into, FromOuter, IntoOuter);
	delete FromOuter;
	delete IntoOuter;
}


#endif