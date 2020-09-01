/*=============================================================================
	CacusBase.h:

	Main include file for CacusLib, must be included first in order to use
	various CacusLib headers/features.

	Author: Fernando Velázquez.
=============================================================================*/

#ifndef USES_CACUS_BASE
#define USES_CACUS_BASE

#include "CacusPlatform.h"


//*******************************************************************
// Character types (overridable)

#ifndef char16
	#define char16 char16_t
#endif
#ifndef char32
	#define char32 char32_t
#endif


//*******************************************************************
// Character stream template

namespace Cacus
{
	template< typename C > inline const C* TGetCharStream( const C* String)
	{
		return String;
	}
	template< typename C , typename S > inline const C* TGetCharStream( const S& String)
	{
		static_assert( false, "TGetCharStream not defined for custom string type." );
		return nullptr;
	}
};


//*******************************************************************
// std::string expansion
#ifdef _STRING_
namespace Cacus
{
	template< typename C > inline const C* TGetCharStream( const std::basic_string<C>& String)
	{
		return String.c_str();
	}
}
#endif


#ifdef CORE_API
#include "Internal/UnrealTournament.h"
#endif




#endif
