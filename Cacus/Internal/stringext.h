#pragma once

#include "../CacusPlatform.h"

#ifdef _STRING_
namespace stringext
{
	//Sets all characters to zero before deallocating
	void CACUS_API SecureCleanup( std::string& Data);

	//Single call file saver
	void CACUS_API SaveFile( const char* OutFilename, const std::string& Data);

	//Single call file loader
	std::string CACUS_API LoadFile( const char* InFilename);
};
#endif