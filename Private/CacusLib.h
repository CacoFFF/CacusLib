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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

#ifdef __GNUC__
	#include <unistd.h>
	#define _stricmp strcasecmp
#endif

#define CACUS_TYPES 1
#define DO_CHECK 1

//***************************************
//          API
#if _WINDOWS
	#if CACUS_BUILDING_LIBRARY
		#define CACUS_API __declspec(dllexport)
	#else
		#define	CACUS_API __declspec(dllimport)
	#endif
#elif _UNIX
	#if __GNUC__ >= 4
		#define CACUS_API __attribute__ ((visibility ("default")))
	#else
		#define CACUS_API
	#endif
#else
	#error "Unrecognized platform"
#endif


//***************************************
//          Memory operations
#define CMemcpy memcpy
#define CMalloc malloc
#define CFree free
#define CMemmove memmove
#define CRealloc realloc


//***************************************
//          Fundamental types
#define CArray std::vector
#define CPair std::pair

#define NPOS std::string::npos

//***************************************
//          Fundamental methods
void DebugCallback( const char* Msg, int MsgType);
