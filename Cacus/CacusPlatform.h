/*=============================================================================
	CacusPlatform.h
	Author: Fernando Vel�zquez

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
#if CACUS_BUILDING_LIBRARY
	#include "../Private/CacusLib.h"
#else
	#include "CacusLib.h"
#endif


#ifndef CACUSLIB_TYPES
											// Unsigned base types.
	typedef unsigned char 		uint8;		// 8-bit  unsigned.
	typedef unsigned short		uint16;		// 16-bit unsigned.
	typedef unsigned int		uint32;		// 32-bit unsigned.
	typedef unsigned long long	uint64;		// 64-bit unsigned.

											// Signed base types.
	typedef	signed char			int8;		// 8-bit  signed.
	typedef signed short int	int16;		// 16-bit signed.
	typedef signed int	 		int32;		// 32-bit signed.
	typedef signed long long	int64;		// 64-bit signed.
	#define CACUSLIB_TYPES

	#ifdef _M_X64
		typedef int64 int_p;
	#elif __LP64__
		typedef int64 int_p;
	#else
	//	typedef unsigned int size_t; //Defined in Visual Studio as native type
		typedef int32 int_p; //Signed int type for dealing with pointers
	#endif
#endif

static_assert( sizeof(uint8)  == 1, "Bad uint8 size");
static_assert( sizeof(uint16) == 2, "Bad uint16 size");
static_assert( sizeof(uint32) == 4, "Bad uint32 size");
static_assert( sizeof(uint64) == 8, "Bad uint64 size");
static_assert( sizeof(int8)   == 1, "Bad int8 size");
static_assert( sizeof(int16)  == 2, "Bad int16 size");
static_assert( sizeof(int32)  == 4, "Bad int32 size");
static_assert( sizeof(int64)  == 8, "Bad int64 size");


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

// NoInit constructor support
#ifndef CACUSLIB_ENUM_NOINIT
	enum ENoInit { E_NoInit = 0 };
#define CACUSLIB_ENUM_NOINIT 1
#endif

// InPlace new operator support
#ifndef CACUSLIB_ENUM_INPLACE
	enum EInPlace { E_InPlace = 0 };
	inline void* operator new( size_t /*Size*/, void* Mem, EInPlace) { return Mem; }
	inline void operator delete( void* /*Ptr*/, void* /*Mem*/, EInPlace) {  }
#define CACUSLIB_ENUM_INPLACE 1
#endif

//C++17 filesystem
#define FILESYSTEM experimental::filesystem::v1

#define STRING2(x) #x  
#define STRING(x) STRING2(x)  

#ifdef _MSC_VER
# pragma warning (disable : 4100) //Unreferenced parameter
# pragma warning (disable : 4127) //Conditional expression is constant
# pragma warning (disable : 4251) //Templated members do not need to be exported you silly compiler
#endif

#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wunknown-pragmas"
# pragma GCC diagnostic ignored "-Wunused-result"
#endif

//Naming convention fix for old GCC
#if defined(__GNUC__) && __GNUC__ < 3
# define FIX_SYMBOL(t) __asm__(#t)
#else
# define FIX_SYMBOL(t)  
#endif

#ifdef _WINDOWS
# define LINUX_SYMBOL(t) 
#else
# define LINUX_SYMBOL(t) __asm__(#t)
#endif

#if (defined(unix) || defined(__unix__) || defined(__unix)) && !defined(_UNIX)
# define _UNIX 1
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
# define nullptr 0
# define constexpr const
# define static_assert(a,b) 
# define NO_CPP11_TEMPLATES
#endif

#ifndef check
# define check(expr) {if (!(expr)) DebugCallback(#expr,CACUS_CALLBACK_EXCEPTION); }
#endif

#ifndef Ccheck
# if DO_CHECK
#  define Ccheck(expr)  {if(!(expr)) CFailAssert( #expr, __FILE__, __LINE__ );}
# else
#  define Ccheck(expr) 0
# endif
#endif

#ifndef _STRING_
# ifdef _BASIC_STRING_H
#  define _STRING_
# endif
#endif

#ifndef _VECTOR_
# ifdef _GLIBCXX_VECTOR
#  define _VECTOR_
# endif
#endif


#endif //USES_CACUS_PLATFORM