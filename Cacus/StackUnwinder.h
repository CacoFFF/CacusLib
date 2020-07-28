#pragma once

#include <setjmp.h>

#if _WINDOWS
	#pragma warning (disable : 4611) //interaction between '_setjmp' and C++ object destruction is non-portable
#endif

extern "C" CACUS_API void CUnwindf( const char* Msg);
extern "C" CACUS_API void CCriticalError( const char* Error);
extern "C" CACUS_API void CFailAssert( const char* Error, const char* File, int32 Line );

//Scoped stack unwinding helper
class CACUS_API CUnwinder
{
public:
	CUnwinder* Prev; //'prev' instead of because it signifies creation time.
	uint32 EnvId;
	int __padding1;
	jmp_buf Environment;
	unsigned long long __padding2; //setjmp appears to be inconsistant between header versions, add this padding just in case

	CUnwinder();
	~CUnwinder();
};



	#define guard(func)	{static const char __FUNC_NAME__[]=#func; \
							CUnwinder Unwinder; \
							try { \
								if ( setjmp(Unwinder.Environment) ) \
									throw 1; \
								else{ \

	#define unguard			}} catch(char*Err) \
							{ \
								CUnwindf(__FUNC_NAME__); \
								throw Err; \
							}catch(...) \
							{ \
								CUnwindf(__FUNC_NAME__); \
								throw; \
							} \
						}

	#define unguardf(msg)	}} catch(char*Err) \
							{ \
								CUnwindf( CSprintf("%s [%s]",__FUNC_NAME__,msg)); \
								throw Err; \
							}catch(...) \
							{ \
								CUnwindf( CSprintf("%s [%s]",__FUNC_NAME__,msg)); \
								throw; \
							} \
						}
