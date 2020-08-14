/*=============================================================================
	Utils.h
	Author: Fernando Velázquez

	Various general purpose utils.
=============================================================================*/

#include "CacusTemplate.h"

#ifndef USES_CACUS_UTILS
#define USES_CACUS_UTILS

namespace cacus
{


	bool CACUS_API ToFile( const char* OutFilename, const uint8* InBuffer, size_t BufferSize);
	bool CACUS_API ToFile( const char* OutFilename, const char* InText);
#ifdef _STRING_
	bool CACUS_API ToFile( const char* OutFilename, const std::string& InData);
#endif

	bool CACUS_API FromFile( const char* InFilename, uint8* OutBuffer, size_t BufferSize, size_t& OutSize, bool NullTerminate=false);
	bool CACUS_API FromFile( const char* InFilename, TFixedArray<uint8>& OutData, bool NullTerminate=false);
#ifdef _STRING_
	bool CACUS_API FromFile( const char* InFilename, std::string& OutData);
#endif


};





#endif