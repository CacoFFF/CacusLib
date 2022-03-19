/*=============================================================================
	Internal/UnrealTournament.h:

	CacusLib expansions for Unreal Tournament

	Author: Fernando Velázquez.
=============================================================================*/

#if !defined(USES_CACUS_BASE) || defined(USES_INTERNAL_UT)
	#error "Internal/UnrealTournament.h must not be included by your project."
#endif

#define USES_INTERNAL_UT
#define CACUSLIB_ENUM_NOINIT 1

namespace Cacus
{
	inline const TCHAR* TGetCharStream( const FString& String)
	{
		return *String;
	}
}
