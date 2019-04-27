
#include "CacusOutputDevice.h"
#include <wchar.h>

void CPrint8( const char* S)
{
	printf("%s",S);
}

void CPrint16( const char16* S)
{
	if ( sizeof(wchar_t) == 2 )
		wprintf( (wchar_t*)S);
	else
	{
		while ( *S )
			wprintf( L"%c", (wchar_t)*S);
	}
}

void CPrint32( const char32* S)
{
	if ( sizeof(wchar_t) == 4 )
		wprintf( (wchar_t*)S);
	else
	{
		while ( *S )
			wprintf( L"%c", (wchar_t)*S);
	}
}

