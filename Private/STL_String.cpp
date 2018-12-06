
#include "Internal/stringext.h"

namespace stringext
{

	void SecureCleanup( std::string& Data)
	{
		if ( Data.size() )
			memset( &Data[0], 0, Data.size() );
		Data.empty();
	}

	void SaveFile( const char* OutFilename, const std::string& Data)
	{
		std::FILE* File = std::fopen( OutFilename, "wb");
		if ( File )
		{
			std::fwrite( &Data[0], 1, Data.size(), File);
			std::fclose( File);
		}
	}

	std::string LoadFile( const char* InFilename)
	{
		std::string Output;
		std::FILE* File = std::fopen( InFilename, "rb");
		if ( File )
		{
			std::fseek( File, 0, SEEK_END);
			Output.resize( std::ftell(File));
			std::rewind( File);
			std::fread( &Output[0], 1, Output.size(), File);
			std::fclose( File);
		}
		return Output;
	}

};