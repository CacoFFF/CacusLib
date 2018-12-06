/*=============================================================================
	BaseDir.cpp
	Author: Fernando Velázquez

	Cross-platform cached base directory.
=============================================================================*/


#include "TCharBuffer.h"
#include "CacusGlobals.h"

static TChar8Buffer<260> BaseDir; //Auto-inits to '\0'
static TChar8Buffer<260> UserDir; //Auto-inits to '\0'

#if _WINDOWS

#pragma comment ( lib, "Wtsapi32.lib")
#include <Windows.h>
#include <WtsApi32.h>
#include <shlobj.h>

const char* CBaseDir()
{
	if( !BaseDir[0] )
	{
		GetModuleFileNameA( GetModuleHandleA(nullptr), *BaseDir, BaseDir.Size() );
		size_t i;
		for( i=BaseDir.Len()-1; i>0; i-- ) //Remove the filename
			if( BaseDir[i-1]=='\\' || BaseDir[i-1]=='/' )
				break;
		BaseDir[i]=0;
	}
	return *BaseDir;
}

const char* CUserDir()
{
	//Makes sure program running as SYSTEM service gets a user folder
	if ( !UserDir[0] )
	{
		HANDLE hToken = nullptr;
		uint32 SessionId = WTSGetActiveConsoleSessionId();
		if ( SessionId != MAXDWORD )
			WTSQueryUserToken(SessionId, &hToken);
		//Needs dynamic linking if this gets deprecated
		SHGetFolderPathA( nullptr, CSIDL_PERSONAL, hToken, SHGFP_TYPE_CURRENT, *UserDir); 
		if ( hToken )
			CloseHandle( hToken);
		if ( !UserDir[0] )
		{
			wchar_t* Res = nullptr;
			SHGetKnownFolderPath( FOLDERID_Documents, 0, nullptr, &Res);
			if ( Res )
				UserDir = Res;
		}
		if ( !UserDir[0] )
			UserDir = "%userprofile%/Documents";
	}
	return *UserDir;
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