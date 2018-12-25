
#include <string>
#include <vector>
#include "CacusTemplate.h"
#include "CacusGlobals.h"
#include "CacusString.h"
#include "CacusOutputDevice.h"
#include "AppTime.h"
#include "Atomics.h"




COutputDevice* ConstructOutputDevice( COutputType Type)
{
	switch (Type)
	{
	case COUT_Null:
		return new COutputDeviceNull();
	case COUT_Printf:
		return new COutputDevicePrintf();
	case COUT_File_UTF8:
		return new COutputDeviceFileUTF8();
	default:
		return nullptr;
	}
}

void DestructOutputDevice( COutputDevice* Device)
{
	if ( Device )
		delete Device;
}


//***********************************************
// COutputDeviceList

COutputDeviceList CLog;

void COutputDeviceList::Init( const char* LocalFilename)
{
	Add( new COutputDeviceFileUTF8(LocalFilename) );
	Add( new COutputDevicePrintf() );
}

void COutputDeviceList::Log( const char* S )
{
	CSpinLock SL(&Lock);
	for ( uint32 i=0 ; i<List.size() ; i++ )
	{
		List[i]->Serialize8(S);
		List[i]->Serialize8("\n");
	}
}

COutputDeviceList& COutputDeviceList::operator<<(const char * C)
{
	CSpinLock SL(&Lock);
	for ( uint32 i=0 ; i<List.size() ; i++ )
		List[i]->Serialize8(C);
	return *this;
}

COutputDeviceList& COutputDeviceList::operator<<(int32 I)
{
	TChar8Buffer<32> Buffer;
	sprintf( *Buffer, "%i", I);
	return (*this) << *Buffer;
}

//***********************************************
// COutputDeviceFile
// Base class of all file type output devices
//
COutputDeviceFile::COutputDeviceFile( const char* InFilename)
	: FilePtr(nullptr)
	, Filename( InFilename)
	, Opened(0)
	, Dead(0)
	, AutoFlush(1)
{}

COutputDeviceFile::~COutputDeviceFile()
{
	Close();
}

void COutputDeviceFile::Open( uint32 AltFilenameAttempts)
{
	if ( FilePtr )
		return;

	const char* WriteFlags = Opened ? "a+b" : "w+b"; //Append if previously opened

	// Separate filename from extension, create missing components if needed
	// This can parse filenames starting with dots just fine (even if they're bad)
	const char* FilenamePart = "";
	const char* ExtensionPart = "";
	for ( const char* Dot=*Filename ; AdvanceTo( Dot, '.') ; ExtensionPart=Dot++ );
	if ( *ExtensionPart ) //Found extension in filename
	{
		FilenamePart = CopyToBuffer( *Filename, ExtensionPart-*Filename);
		ExtensionPart = CopyToBuffer( ExtensionPart); 
	}
	else //Filename has no extension
	{
		FilenamePart = CopyToBuffer( *Filename);
		ExtensionPart = ".log";
	}
	if ( !*FilenamePart ) //Filename is empty
		FilenamePart = "Cacus";

	//Allocate large enough buffer
	char* FinalFilename = (char*)CMalloc( CStrlen(FilenamePart) + CStrlen(ExtensionPart) + 10);
	sprintf( FinalFilename, "%s%s", FilenamePart, ExtensionPart);
	FilePtr = std::fopen( FinalFilename, WriteFlags);
	for ( uint32 i=0 ; !FilePtr && i<AltFilenameAttempts ; i++ ) //Add _2 if necessary
	{
		sprintf( FinalFilename, "%s_%i%s", FilenamePart, (int)(i+2), ExtensionPart);
		FilePtr = std::fopen( FinalFilename, WriteFlags);
	}

	Dead = (FilePtr == nullptr);
	Opened += (FilePtr != nullptr);
}

void COutputDeviceFile::Close()
{
	if ( FilePtr )
	{
		Flush();
		std::fclose( (std::FILE*)FilePtr);
		FilePtr = nullptr;
	}
}

void COutputDeviceFile::SetFilename( const char* InFilename)
{
	Close();
	Filename = InFilename;
}

void COutputDeviceFile::SetFilename( const wchar_t* InFilename)
{
	Close();
	Filename = InFilename;
}

void COutputDeviceFile::Flush()
{
	if( FilePtr )
		std::fflush( (std::FILE*)FilePtr);
}



//***********************************************
// COutputDeviceFileUTF8

static const uint8 UTF8_BOM[3] = { 0xEF, 0xBB, 0xBF }; 
bool COutputDeviceFileUTF8::Init()
{
	if( !FilePtr && !Dead )
	{
		Open(32);
		if ( FilePtr )
			std::fwrite( UTF8_BOM, 1, 3, (std::FILE*)FilePtr);
	}
	return FilePtr != nullptr;
}

void COutputDeviceFileUTF8::Write( const char* UTF8Stream)
{
	if ( UTF8Stream && *UTF8Stream )
	{
		if ( *UTF8Stream == '\n' ) //Hack: write the carriage return if missing
			std::fwrite( "\r", 1, 1, (std::FILE*)FilePtr);
		size_t len = CStrlen(UTF8Stream);
		std::fwrite( UTF8Stream, 1, len, (std::FILE*)FilePtr);
	}
}

#define COUT_UTF8_STANDARD \
	if ( *Data && Init() ) \
	{ \
		char Buffer[512]; \
		while ( true ) \
		{ \
			size_t EncodeStatus = utf8::Encode( Buffer, Data); /*Encode as much as we can onto this limited buffer*/ \
			Write( Buffer); \
			if ( EncodeStatus == 0 ) /*Data was fully encoded+logged*/ \
				break; \
			Data += EncodeStatus; /*If not, EncodeStatus indicates offset where to keep encoding*/ \
		} \
		if ( AutoFlush ) \
			Flush(); \
	}

void COutputDeviceFileUTF8::Serialize8( const char* Data)
{
	COUT_UTF8_STANDARD
}

void COutputDeviceFileUTF8::Serialize16( const char16* Data)
{
	COUT_UTF8_STANDARD
}

void COutputDeviceFileUTF8::Serialize32( const char32* Data)
{
	COUT_UTF8_STANDARD
}