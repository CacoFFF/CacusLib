#ifndef USES_CACUS_TESTS
#define USES_CACUS_TESTS

#include "CacusPlatform.h"

extern "C" CACUS_API void TestCallbacks();
extern "C" CACUS_API void TestCharBuffer();
extern "C" CACUS_API void TestTimer();

inline void TestMain()
{
	#define TEST_AND_CONTINUE(testfunc) try{ testfunc(); } catch(...){}
	TEST_AND_CONTINUE(TestCallbacks)
	TEST_AND_CONTINUE(TestCharBuffer)
	TEST_AND_CONTINUE(TestTimer)
	#undef TEST_AND_CONTINUE
}




#endif
