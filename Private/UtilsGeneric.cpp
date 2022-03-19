/*=============================================================================
	UtilsGeneric.cpp
	Author: Fernando Velázquez

	Various general purpose utils implementations.
=============================================================================*/

#include "CacusLibPrivate.h"

#include "Utils.h"
#include "CacusString.h"
#include "DebugCallback.h"




bool CACUS_API cacus::ToFile( const char* OutFilename, const uint8* Data, size_t Bytes)
{
	std::FILE* File = std::fopen( OutFilename, "wb");
	if ( File )
	{
		size_t Written = 0;
		if ( Bytes )
		{
			Written = std::fwrite( &Data[0], 1, Bytes, File);
			if ( Written != Bytes )
				DebugCallback( CSprintf("Failed to write to %s, (%i/%i bytes written)", OutFilename, (int)Written, (int)Bytes), CACUS_CALLBACK_IO);
		}
		std::fclose( File);
		return (Written == Bytes);
	}
	else
		DebugCallback( CSprintf("Failed to open file for write %s", OutFilename), CACUS_CALLBACK_IO);
	return false;
}

bool cacus::ToFile( const char* OutFilename, const char* TextData)
{
	return cacus::ToFile( OutFilename, (uint8*)TextData, CStrlen(TextData));
}

#ifdef _STRING_
bool cacus::ToFile( const char* OutFilename, const std::string& Data)
{
	return cacus::ToFile( OutFilename, (uint8*)Data.c_str(), Data.length());
}
#endif



static size_t GetFileSizeAndRewind( std::FILE* File)
{
	size_t FileSize;
	std::fseek( File, 0, SEEK_END);
	FileSize = std::ftell(File);
	std::rewind(File);
	return FileSize;
}

bool CACUS_API cacus::FromFile( const char* InFilename, uint8* OutBuffer, size_t BufferSize, size_t& OutSize, bool NullTerminate)
{
	OutSize = 0;
	std::FILE* File = std::fopen( InFilename, "rb");
	if ( File )
	{
		size_t FileSize = GetFileSizeAndRewind(File);
		size_t NullSize = size_t(NullTerminate != 0);
		if ( FileSize + NullSize > BufferSize )
			DebugCallback( CSprintf("Insufficient buffer for reading %s, (%i/%i bytes read)", InFilename, (int)BufferSize, (int)FileSize), CACUS_CALLBACK_IO);
		OutSize = std::fread( OutBuffer, 1, Min(BufferSize,FileSize), File);
		if ( NullSize )
			OutBuffer[Min(FileSize,BufferSize-1)] = '\0';
		std::fclose( File);
		return OutSize == FileSize;
	}
	else
		DebugCallback( CSprintf("Failed to open file for read %s", InFilename), CACUS_CALLBACK_IO);
	return false;
}

bool CACUS_API cacus::FromFile( const char* InFilename, TFixedArray<uint8>& OutData, bool NullTerminate)
{
	OutData.Setup(0);
	std::FILE* File = std::fopen( InFilename, "rb");
	if ( File )
	{
		size_t FileSize = GetFileSizeAndRewind(File);
		size_t NullSize = size_t(NullTerminate != 0);
		OutData.Setup(FileSize + NullSize);
		size_t OutSize = std::fread( OutData.GetData(), 1, FileSize, File);
		if ( NullSize )
			OutData[FileSize] = '\0';
		std::fclose( File);
		return OutSize == FileSize;
	}
	else
		DebugCallback( CSprintf("Failed to open file for read %s", InFilename), CACUS_CALLBACK_IO);
	return false;
}

#ifdef _STRING_
bool CACUS_API cacus::FromFile( const char* InFilename, std::string& OutData)
{
	OutData.clear();
	std::FILE* File = std::fopen( InFilename, "rb");
	if ( File )
	{
		size_t FileSize = GetFileSizeAndRewind(File);
		OutData.resize(FileSize);
		size_t OutSize = std::fread( &OutData[0], 1, FileSize, File);
		std::fclose( File);
		return OutSize == FileSize;
	}
	return false;
}
#endif
