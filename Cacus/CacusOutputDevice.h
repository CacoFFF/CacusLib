#pragma once

#include "CacusPlatform.h"
#include "TCharBuffer.h"

class CACUS_API COutputDevice
{
public:
	virtual ~COutputDevice() {}
	virtual void Serialize8 ( const char* S)=0;
	virtual void Serialize16( const char16* S)=0;
	virtual void Serialize32( const char32* S)=0;

	template<typename CHAR> void Serialize( const CHAR* S)
	{
		if      ( sizeof(CHAR) == 1 ) Serialize8 ( (const char*)  S);
		else if ( sizeof(CHAR) == 2 ) Serialize16( (const char16*)S);
		else if ( sizeof(CHAR) == 4 ) Serialize32( (const char32*)S);
	}
};

enum COutputType
{
	COUT_Null,
	COUT_Printf,
	COUT_File_ANSI,
	COUT_File_UTF8
};

extern "C"
{
	//If you want to avoid C++ linkage
	CACUS_API COutputDevice* ConstructOutputDevice( COutputType Type);
	CACUS_API void           DestructOutputDevice( COutputDevice* Device);
};


#ifdef _VECTOR_

#include "CacusTemplate.h"
//TODO: MAKE THIS CUSTOMIZABLE AND NOT GLOBAL
class CACUS_API COutputDeviceList : public TMasterObjectArray<COutputDevice*>
{
public:
	void Init(  const char* LocalFilename=nullptr);

	void Log( const char* S );

	COutputDeviceList& operator<<( const char* C);
	COutputDeviceList& operator<<( int32 I);
	#ifdef _STRING_
	COutputDeviceList& operator<<( const std::string& Str) { return (*this) << Str.c_str(); }
	#endif
};
extern "C" CACUS_API COutputDeviceList CLog;
#define Cdebug(text) CLog.Log( text )
#define Cdebugf(...) CLog.Log( CSprintf(__VA_ARGS__) )

#endif

class CACUS_API COutputDeviceNull : public COutputDevice
{
public:
	COutputDeviceNull() {}
	virtual void Serialize8 ( const char* S)  {};
	virtual void Serialize16( const char16* S) {};
	virtual void Serialize32( const char32* S) {};
};

class CACUS_API COutputDeviceFile : public COutputDevice
{
public:
	COutputDeviceFile( const char* InFilename = nullptr);
	~COutputDeviceFile();

protected:
	void*              FilePtr;
	TCharBuffer<1024,char> Filename;
public:
	uint32             Opened;
	uint32             Dead;
	uint32             AutoFlush;

public:
	void Open( uint32 AltFilenameAttempts=0);
	void Close();
	void SetFilename( const char* InFilename);
	void SetFilename( const wchar_t* InFilename);
	void Flush();
};

//UTF-8 exporter (generalize later)
class CACUS_API COutputDeviceFileUTF8 : public COutputDeviceFile
{
public:
	COutputDeviceFileUTF8( const char* InFilename = nullptr) : COutputDeviceFile(InFilename) {}

	void Serialize8 ( const char* Data);
	void Serialize16( const char16* Data);
	void Serialize32( const char32* Data);

private:
	bool Init();
	void Write( const char* UTF8Stream);
};


//PrintF output device
extern "C"
{
//Wide print to console
	CACUS_API void CPrint8 ( const char*   S);
	CACUS_API void CPrint16( const char16* S);
	CACUS_API void CPrint32( const char32* S);
}

template <typename CHAR> void CPrint( const CHAR* S)
{
	if      ( sizeof(CHAR) == 1 ) CPrint8 ( (const char*)  S);
	else if ( sizeof(CHAR) == 2 ) CPrint16( (const char16*)S);
	else if ( sizeof(CHAR) == 4 ) CPrint32( (const char32*)S);
}

class CACUS_API COutputDevicePrintf : public COutputDevice
{
public:
	COutputDevicePrintf() {};

	void Serialize8 ( const char* S)    { CPrint8 (S); }
	void Serialize16( const char16* S)  { CPrint16(S); }
	void Serialize32( const char32* S)  { CPrint32(S); }
};
