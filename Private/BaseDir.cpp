/*=============================================================================
	BaseDir.cpp
	Author: Fernando Velázquez

	Cross-platform cached base directory.
=============================================================================*/

#include "CacusBase.h"
#include "TCharBuffer.h"
#include "CacusGlobals.h"

static TChar8Buffer<260> BaseDir; //Auto-inits to '\0'
static TChar8Buffer<260> UserDir; //Auto-inits to '\0'

#if _WINDOWS

//#pragma comment ( lib, "Wtsapi32.lib")
#include <Windows.h>
#include <WtsApi32.h>
#include <shlobj.h>

const char* CBaseDir()
{
	if( !BaseDir[0] )
	{
		GetModuleFileNameA( GetModuleHandleA(nullptr), *BaseDir, (DWORD)BaseDir.Size() );
		size_t i;
		for( i=BaseDir.Len()-1; i>0; i-- ) //Remove the filename
			if( BaseDir[i-1]=='\\' || BaseDir[i-1]=='/' )
				break;
		BaseDir[i]=0;
	}
	return *BaseDir;
}


static HANDLE GetUserToken(); 
static void GetKnownDocumentsPath( HANDLE hToken);
const char* CUserDir()
{
	//Makes sure program running as SYSTEM service gets a user folder
	if ( !UserDir[0] )
	{
		HANDLE hToken = GetUserToken();
		//Needs dynamic linking if this gets deprecated
		SHGetFolderPathA( nullptr, CSIDL_PERSONAL, hToken, SHGFP_TYPE_CURRENT, *UserDir); 
		if ( !UserDir[0] )
			GetKnownDocumentsPath( hToken);
		if ( hToken )
			CloseHandle( hToken);
		if ( !UserDir[0] )
			UserDir = "%userprofile%\\Documents";
	}
	return *UserDir;
}

// Helper
struct ScopedLibrary
{
	HMODULE Handle;

	ScopedLibrary( const char* LibraryName)            : Handle( LoadLibraryA(LibraryName)) {}
	~ScopedLibrary()                                   { if ( Handle ) FreeLibrary( Handle); }

	operator bool() const                              { return Handle != nullptr; }
	template<class T> T Get( const char* Sym) const    { return (T)GetProcAddress( Handle, Sym); }
};
#define IF_LOADED_LIBRARY(lib) ScopedLibrary lib( STRING(lib) ".dll"); if ( lib )

//If running as SYSTEM service, get token needed to impersonate user
typedef DWORD (__stdcall *dw_func_v)();
typedef BOOL (__stdcall *i_func_dw_pv)(uint32,HANDLE);
static HANDLE GetUserToken()
{
	HANDLE hToken = nullptr;
	IF_LOADED_LIBRARY(Kernel32)
	{
		auto hWTSGetActiveConsoleSessionId = Kernel32.Get<dw_func_v>("WTSGetActiveConsoleSessionId");
		if ( hWTSGetActiveConsoleSessionId )
		{
			uint32 SessionId = (*hWTSGetActiveConsoleSessionId)();
			if ( SessionId != MAXDWORD )
			{
				IF_LOADED_LIBRARY(Wtsapi32)
				{
					auto hWTSQueryUserToken = Wtsapi32.Get<i_func_dw_pv>("WTSQueryUserToken");
					if ( hWTSQueryUserToken )
						(*hWTSQueryUserToken)(SessionId, &hToken);
				}
			}
		}
	}
	return hToken;
}



//Attempt to get the documents folder using a >Vista entry point
typedef HRESULT (*l_func_kfid_dw_pv_ppcwc)( const KNOWNFOLDERID&, uint32, HANDLE, wchar_t**);
static void GetKnownDocumentsPath( HANDLE hToken)
{
	wchar_t* Res = nullptr;
	IF_LOADED_LIBRARY(Shell32)
	{
		auto hSHGetKnownFolderPath = Shell32.Get<l_func_kfid_dw_pv_ppcwc>("SHGetKnownFolderPath");
		if ( hSHGetKnownFolderPath )
			(*hSHGetKnownFolderPath)( FOLDERID_Documents, 0, hToken, &Res);
	}
	if ( Res )
	{
		UserDir = Res;
		CoTaskMemFree( Res);
	}
}




#elif __GNUC__

#include <unistd.h>
#include <pwd.h>

const char* CBaseDir()
{
	if( !BaseDir[0] )
	{
		getcwd( *BaseDir, BaseDir.Size()-1 );
		strcat( *BaseDir, "/"); //Append needed '/'
	}
	return *BaseDir;
}

const char* CUserDir()
{
	if ( !UserDir[0] )
	{
		char* home = getenv("HOME");
		if ( !home )
		{
			struct passwd *pw = getpwuid(getuid());
			if (pw)
				home = pw->pw_dir;
		}
		if ( home )
			UserDir = home;
	}
	return *UserDir;
}

#endif