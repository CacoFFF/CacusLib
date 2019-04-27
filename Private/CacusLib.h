/**
	This header is included in EVERY cpp file before CacusPlatform is included
	It's purpose is to setup the library for use in both App and Cacus

	API must correspond to import/export (at least in windows)

	This is the export version of CacusLib.h
	Only CacusLib should use this header.
*/
#pragma once

/*
_WINDOWS
_CRT_SECURE_NO_WARNINGS
*/

#define DISABLE_STDLIB11_FEATURES 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !DISABLE_STDLIB11_FEATURES
	#include <vector>
	#include <string>
#endif

#ifdef __GNUC__
	#include <unistd.h>
	#define _stricmp strcasecmp
#endif

#define CACUS_TYPES 1
#define DO_CHECK 1

//***************************************
//          API
#if _WINDOWS
	#define CACUS_API __declspec(dllexport)
#else
	#if __GNUC__ >= 4
		#define CACUS_API __attribute__ ((visibility ("default")))
	#else
		#define CACUS_API
	#endif
#endif


//***************************************
//          Memory operations
#define CMemcpy memcpy
#define CMalloc malloc
#define CFree free
#define CMemmove memmove


//***************************************
//          Fundamental types
#define CArray std::vector
#define CPair std::pair

#define NPOS std::string::npos

//***************************************
//          Fundamental methods
void DebugCallback( const char* Msg, int MsgType);
