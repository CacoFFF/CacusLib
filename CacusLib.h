/**
	This header is included in EVERY cpp file before CacusPlatform is included
	It's purpose is to setup the library for use in both App and Cacus

	API must correspond to import/export (at least in windows)

	This is the import version of CacusLib.h
	Use this (or modify) to link your program/library with CacusLib
*/
#ifdef USES_CACUSLIB
	#error CacusLib.h (import version) should only be included through CacusPlatform.h
#endif
#define USES_CACUSLIB

/*
_WINDOWS
_CRT_SECURE_NO_WARNINGS
*/

#ifdef __GNUC__
	#include <string.h>
	#include <unistd.h>
	#define _stricmp strcasecmp
#endif

#define DO_CHECK 1

#if (__cplusplus < 201103L) && (defined(_MSC_VER) && _MSC_VER < 1600) || (defined(__GNUC__) && __GNUC__ < 5)
	#define DISABLE_CPP11
#endif

//***************************************
//          API
#ifndef CACUS_API
	#if _WINDOWS
		#define CACUS_API __declspec(dllimport)
	#else
		#if __GNUC__ >= 4
			#define CACUS_API __attribute__ ((visibility ("default")))
		#else
			#define CACUS_API
		#endif
	#endif
#endif

//***************************************
//          Memory operations
#define CMemcpy memcpy
#define CMalloc malloc
#define CFree free


//***************************************
//          Fundamental types
#define CArray std::vector
#define CPair std::pair

#define NPOS std::string::npos


//***************************************
//          Using with: Unreal Engine (old)
#ifdef _INC_CORE
#ifdef CORE_API
/*	#define uint8  BYTE
	#define uint16 _WORD
	#define uint32 DWORD
	#define uint64 QWORD
	#define int8  SBYTE
	#define int16 SWORD
	#define int32 INT
	#define int64 SQWORD
	#define CACUSLIB_TYPES*/
	#define CACUSLIB_ENUMS
/*	#undef FORCEINLINE
	#ifdef DISABLE_CPP11
		#define char16 _WORD
		#define char32 DWORD
	#endif*/
#endif
#endif

