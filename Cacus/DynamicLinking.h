/*=============================================================================
	DynamicLinking.h
	Author: Fernando Velázquez

	Helpers for library loading and handle acquisition.
=============================================================================*/
#pragma once

#if _WINDOWS

extern "C"
{
__declspec(dllimport) void* __stdcall LoadLibraryA( const char* lpLibFileName);
__declspec(dllimport) int   __stdcall FreeLibrary( void* hLibModule);
__declspec(dllimport) void* __stdcall GetProcAddress( void* hModule, const char* lpProcName);
}

struct CWindowsScopedLibrary
{
	void* Handle;

	CWindowsScopedLibrary()                            : Handle(nullptr) {}
	CWindowsScopedLibrary( const char* LibraryName)    : Handle( LoadLibraryA(LibraryName)) {}
	~CWindowsScopedLibrary()                           { if ( Handle ) FreeLibrary( Handle); }

	operator bool() const                              { return Handle != nullptr; }
	template<class T> T Get( const char* Sym) const    { return (T)GetProcAddress( Handle, Sym); }
};
//#define IF_LOADED_LIBRARY(lib) CScopedLibrary lib( STRING(lib) ".dll"); if ( lib )

typedef CWindowsScopedLibrary CScopedLibrary;

#endif

