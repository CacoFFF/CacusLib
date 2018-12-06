
#ifndef USES_CACUS_GLOBALS
#define USES_CACUS_GLOBALS

#include "CacusPlatform.h"

extern CACUS_API int32 volatile COpenThreads;

extern "C"
{
	// Both are 260-char buffers, modify at own risk
	CACUS_API const char* CBaseDir();
	CACUS_API const char* CUserDir(); //Or Home dir
}

#endif
