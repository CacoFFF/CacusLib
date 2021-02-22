/*=============================================================================
	DynamicLinking.h
	Author: Fernando Velázquez

	Helpers for library loading and handle acquisition.
=============================================================================*/
#pragma once

#if _WINDOWS

# ifndef _INC_WINDOWS
extern "C"
{
__declspec(dllimport) void* __stdcall LoadLibraryA( const char* lpLibFileName);
__declspec(dllimport) int   __stdcall FreeLibrary( void* hLibModule);
__declspec(dllimport) void* __stdcall GetProcAddress( void* hModule, const char* lpProcName);
}
# endif

#define CACUSLIB_LIBRARY_EXTENSION ".dll"

struct CWindowsScopedLibrary
{
# ifndef _INC_WINDOWS
	void* Handle;
# else
	HMODULE Handle;
# endif

	CWindowsScopedLibrary()                            : Handle(nullptr) {}
	CWindowsScopedLibrary( const char* LibraryName)    : Handle(LoadLibraryA(LibraryName)) {}
	CWindowsScopedLibrary( CWindowsScopedLibrary&& Rhs){ Handle=Rhs.Handle; Rhs.Handle=nullptr; }
	~CWindowsScopedLibrary()                           { if ( Handle ) FreeLibrary( Handle); }

	operator bool() const                              { return Handle != nullptr; }
	template<class T> T Get( const char* Sym) const    { return (T)GetProcAddress( Handle, Sym); }
};
//#define IF_LOADED_LIBRARY(lib) CScopedLibrary lib( STRING(lib) ".dll"); if ( lib )

typedef CWindowsScopedLibrary CScopedLibrary;

#elif _UNIX

#define CACUSLIB_LIBRARY_EXTENSION ".so"

#include <dlfcn.h>
struct CUnixScopedLibrary
{
	void* Handle;

	CUnixScopedLibrary()                               : Handle(nullptr) {}
	CUnixScopedLibrary( const char* LibraryName)       : Handle(dlopen(LibraryName, RTLD_NOW|RTLD_LOCAL)) {}
	CUnixScopedLibrary( CUnixScopedLibrary&& Rhs)      { Handle=Rhs.Handle; Rhs.Handle=nullptr; }
	~CUnixScopedLibrary()                              { if ( Handle ) dlclose(Handle); }

	operator bool() const                              { return Handle != nullptr; }
	template<class T> T Get( const char* Sym) const    { return (T)dlsym( Handle, Sym); }
};
typedef CUnixScopedLibrary CScopedLibrary;

#endif

