/*=============================================================================
	CacusPlatform.h
	Author: Fernando Velázquez

	Defines for new datatypes.
	Designed to make UE4 code and ports work without edition.
=============================================================================*/

#ifndef USES_CACUS_PLATFORM
#define USES_CACUS_PLATFORM

#undef BYTE
#undef WORD
#undef DWORD
#undef INT
#undef FLOAT
#undef MAXBYTE
#undef MAXWORD
#undef MAXDWORD
#undef MAXINT
#undef VOID
#undef CDECL


//Private CacusLib.h header, each project should have it's own adaptation.
#include "CacusLib.h"


#ifndef CACUSLIB_TYPES
											// Unsigned base types.
	typedef unsigned char 		uint8;		// 8-bit  unsigned.
	typedef unsigned short		uint16;		// 16-bit unsigned.
	typedef unsigned long		uint32;		// 32-bit unsigned.
	typedef unsigned long long	uint64;		// 64-bit unsigned.

											// Signed base types.
	typedef	signed char			int8;		// 8-bit  signed.
	typedef signed short int	int16;		// 16-bit signed.
	typedef signed int	 		int32;		// 32-bit signed.
	typedef signed long long	int64;		// 64-bit signed.
	#define CACUSLIB_TYPES

	#ifdef __WIN64
	//	typedef uint64 size_t;
		typedef int64 int_p;
	#elif __LP64__
		typedef uint64 size_t;
		typedef int64 int_p;
	#else
	//	typedef unsigned int size_t; //Defined in Visual Studio as native type
		typedef int32 int_p; //Signed int type for dealing with pointers
	#endif
#endif


#ifndef CACUSLIB_ENUMS
	enum {MAXBYTE		= 0xff       };
	enum {MAXWORD		= 0xffffU    };
	enum {MAXDWORD		= 0xffffffffU};
	enum {MAXSBYTE		= 0x7f       };
	enum {MAXSWORD		= 0x7fff     };
	enum {MAXINT		= 0x7fffffff };
	enum {INDEX_NONE	= -1l        };
	#define CACUSLIB_ENUMS
#endif

//C++17 filesystem
#define FILESYSTEM experimental::filesystem::v1

#define STRING2(x) #x  
#define STRING(x) STRING2(x)  

#ifdef _MSC_VER
	#pragma warning (disable : 4100) //Unreferenced parameter
	#pragma warning (disable : 4127) //Conditional expression is constant
	#pragma warning (disable : 4251) //Templated members do not need to be exported you silly compiler
#endif

#ifdef __GNUC__
	#pragma GCC diagnostic ignored "-Wunknown-pragmas"
	#pragma GCC diagnostic ignored "-Wunused-result"
#endif

//Naming convention fix for old GCC
#if defined(__GNUC__) && __GNUC__ < 3
	#define FIX_SYMBOL(t) __asm__(#t)
#else
	#define FIX_SYMBOL(t)  
#endif

#ifdef _WINDOWS
	#define LINUX_SYMBOL(t) 
#else
	#define LINUX_SYMBOL(t) __asm__(#t)
#endif


//Operating system dependant
#ifdef _WINDOWS
	#ifndef FORCEINLINE
		#define FORCEINLINE __forceinline
	#endif
	#define VARARGS __cdecl
	#define CDECL __cdecl
	#define STDCALL __stdcall
#elif __GNUC__
	#ifndef FORCEINLINE
		#if __GNUC__ >= 3
			#define FORCEINLINE __attribute__((always_inline)) inline
		#else
			#define FORCEINLINE inline
		#endif
	#endif
	#define VARARGS
	#define CDECL
	#define STDCALL
#endif

//Compiler dependant
//C++11 features
#ifdef DISABLE_CPP11
	#define nullptr 0
	#define constexpr const
	#define static_assert(a,b) 
	#define NO_CPP11_TEMPLATES
#endif


#ifndef Ccheck
	#if DO_CHECK
		#define Ccheck(expr)  {if(!(expr)) CFailAssert( #expr, __FILE__, __LINE__ );}
	#else
		#define Ccheck(expr) 0
	#endif
#endif

#ifndef _STRING_
	#ifdef _BASIC_STRING_H
		#define _STRING_
	#endif
#endif

#ifndef _VECTOR_
	#ifdef _GLIBCXX_VECTOR
		#define _VECTOR_
	#endif
#endif


#endif //USES_CACUS_PLATFORM