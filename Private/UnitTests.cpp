

#include "CacusLibPrivate.h"

#include "UnitTests.h"

#ifndef CACUS_USE_TESTS

void TestCallbacks(){}
void TestCharBuffer(){}
void TestTimer(){}

#else


#include "DebugCallback.h"
#include "TCharBuffer.h"
#include "CTickerEngine.h"

#include <stdio.h>


//======================================================================
//========================= CALLBACKS ==================================
//======================================================================
static void MainCallback( const char* Message, int MessageFlags)
{
	if ( *Message )
		printf( "%s\n", Message);
}

static int TestCallbackCount = 0;
static void TestCallback( const char* Message, int MessageFlags)
{
	TestCallbackCount++;
}

static int DbgCallbackCount = 0;
static void DbgCallback( const char* Message, int MessageFlags)
{
	DbgCallbackCount++;
}

static void ExceptCallback( const char* Message, int MessageFlags)
{
	throw Message;
}

#define ORDERED_CALLBACKS 4
static int Order = 0;
template<int N> void OrderedCallback( const char* Message, int MessageFlags)
{
	if ( Order != N )
	{
		static char Buf[64];
		sprintf( Buf, "Order error [%i <> %i]", N, Order);
		throw Buf;
	}

	if ( ++Order == ORDERED_CALLBACKS )
		Order = 0;
}

//======================================================================
//============================= TESTS ==================================
//======================================================================

#define guardtest(teststring) \
	printf( "Testing " teststring "... "); \
	const char* Stage = "Start"; \
	try { \
		CDbg_RegisterCallback( &MainCallback, CACUS_CALLBACK_ALL); \
		CDbg_RegisterCallback( &ExceptCallback, CACUS_CALLBACK_EXCEPTION);

#define unguardtest \
	printf("OK\n"); \
	} catch( const char* Msg) { \
		printf("ERROR: %s (stage %s)\n", Msg, Stage); } \
	CDbg_UnregisterCallback( &MainCallback); \
	CDbg_UnregisterCallback( &ExceptCallback);

static char ThrowBuf[256] = {};
#define checktest(statement,...) if ( !(statement) ) { sprintf(ThrowBuf,##__VA_ARGS__); throw (const char*)ThrowBuf; }


//============================= TestCallbacks
//
void TestCallbacks()
{
	guardtest("Callback system");
	CDbg_RegisterCallback( &TestCallback, CACUS_CALLBACK_ALL);
	CDbg_RegisterCallback( &DbgCallback, CACUS_CALLBACK_DEBUGALL);
	CDbg_UnregisterCallback( &MainCallback); //It's autoregistered by guard

	Stage = "Loops";
	TestCallbackCount = DbgCallbackCount = 0;
	int ExpectedTestCallbacks = 200 + rand() % 200;
	int ExpectedDbgCallbacks = 200 + rand() % 200;
	for ( int i=0 ; i<ExpectedTestCallbacks ; i++ ) DebugCallback( "", CACUS_CALLBACK_TEST);
	for ( int i=0 ; i<ExpectedDbgCallbacks  ; i++ ) DebugCallback( "", CACUS_CALLBACK_TEST|CACUS_CALLBACK_DEBUGONLY);
	checktest( TestCallbackCount == ExpectedTestCallbacks, "Callback count error [TEST] [%i/%i]", TestCallbackCount, ExpectedTestCallbacks);
	checktest( DbgCallbackCount  == ExpectedDbgCallbacks , "Callback count error [DBG] [%i/%i]" , DbgCallbackCount,  ExpectedDbgCallbacks);

	Stage  = "Exception";
	int TestException = 0;
	try
	{
		DebugCallback( "Testing exception callback...", CACUS_CALLBACK_EXCEPTION);
	}
	catch(...)
	{
		TestException++;
	}
	if ( !TestException )
		throw "Exception callback";

	Stage = "Order";
	CDbg_UnregisterCallback( &TestCallback);
	CDbg_UnregisterCallback( &DbgCallback);
	CDbg_UnregisterCallback( &ExceptCallback);
	DebugCallback( "This message should NOT appear here", CACUS_CALLBACK_EXCEPTION|CACUS_CALLBACK_DEBUGONLY);
	CDbg_RegisterCallback( &ExceptCallback, CACUS_CALLBACK_EXCEPTION);
	DebugCallback( "Testing callback ordering...", CACUS_CALLBACK_TEST);
	CDbg_RegisterCallback( &OrderedCallback<0>, CACUS_CALLBACK_DEBUGALL);
	CDbg_RegisterCallback( &OrderedCallback<1>, CACUS_CALLBACK_DEBUGALL);
	CDbg_RegisterCallback( &OrderedCallback<2>, CACUS_CALLBACK_DEBUGALL);
	CDbg_RegisterCallback( &OrderedCallback<3>, CACUS_CALLBACK_DEBUGALL);
	DebugCallback( "FIRST ORDER TEST", CACUS_CALLBACK_TEST|CACUS_CALLBACK_DEBUGONLY);
	CDbg_UnregisterCallback( &OrderedCallback<0>);
	CDbg_UnregisterCallback( &OrderedCallback<1>);
	CDbg_UnregisterCallback( &OrderedCallback<2>);
	//Callback <3> is now first one
	int FirstCallback = CDbg_SlotCallback( &OrderedCallback<3>);
	CDbg_RegisterCallback( &OrderedCallback<1>, CACUS_CALLBACK_DEBUGALL);
	CDbg_RegisterCallback( &OrderedCallback<2>, CACUS_CALLBACK_DEBUGALL);

	CDbg_RegisterCallback( &OrderedCallback<0>, CACUS_CALLBACK_DEBUGALL, FirstCallback); //Put zero before <3>
	CDbg_RegisterCallback( &OrderedCallback<3>, CACUS_CALLBACK_DEBUGALL); //Move 3 to last
	DebugCallback( "SECOND ORDER TEST", CACUS_CALLBACK_TEST|CACUS_CALLBACK_DEBUGONLY);
	unguardtest
}


//============================= TestCharBuffer
//
#define TEST_ASSIGNMENT "This is a test string"
void TestCharBuffer()
{
	guardtest("CharBuffer");
	TChar8Buffer<512> AnsiBuffer;
	Stage = "Assignment and test";
	AnsiBuffer = TEST_ASSIGNMENT;
	checktest( strcmp(*AnsiBuffer, TEST_ASSIGNMENT)== 0, "Wrong assignment > %s / " TEST_ASSIGNMENT, *AnsiBuffer);
	checktest( AnsiBuffer.Len() == _len(TEST_ASSIGNMENT), "Wrong length [%i dyn/%i const]", (int)AnsiBuffer.Len(), (int)_len(TEST_ASSIGNMENT) );
	AnsiBuffer += *AnsiBuffer;
	checktest( AnsiBuffer.Len() == _len(TEST_ASSIGNMENT)*2, "Bad self-concatenation [%i dyn/%i const]", (int)AnsiBuffer.Len(), (int)_len(TEST_ASSIGNMENT TEST_ASSIGNMENT) );
	checktest( AnsiBuffer == TEST_ASSIGNMENT TEST_ASSIGNMENT, "Bad self-concatenation [%s/%s]", *AnsiBuffer, TEST_ASSIGNMENT TEST_ASSIGNMENT );

	Stage = "Circular string buffer";
	CStringBufferInit( 32 * 1024); //Init 32kb buffer
	const char* TestC = CSprintf( "%s%s", TEST_ASSIGNMENT, TEST_ASSIGNMENT);
	checktest( AnsiBuffer == TestC, "Bad CSprintf result [%s != %s]", TestC, *AnsiBuffer);
	checktest( !CStrncmp( TestC, TEST_ASSIGNMENT), "CStrncmp <array template> error" );
	unguardtest
}


//============================= TestTimer
// Tests the timing system and sleep system with up to 1 ms error
//
void TestTimer()
{
	guardtest("Timer");
	double AccumulatedTime;
	CTickerEngine Ticker;
	Ticker.UpdateTimerResolution();

	printf( " TR=%fms SR=%fms... ", Ticker.GetTimeStampResolution() * 1000, Ticker.GetSleepResolution() * 1000);

	Stage = "Now";
	Ticker.TickNow();
		
	Stage = "Absolute";
	AccumulatedTime  = Ticker.TickAbsolute( Ticker.GetLastTickTimestamp() + 0.1);
	AccumulatedTime += Ticker.TickAbsolute( Ticker.GetLastTickTimestamp() + 0.1);
	checktest( AccumulatedTime > 0.199 && AccumulatedTime < 0.201, "Ticker engine too imprecise (%f/0.2)", AccumulatedTime);

	Stage = "Interval";
	AccumulatedTime  = Ticker.TickInterval(0.1);
	AccumulatedTime += Ticker.TickInterval(0.1);
	checktest( AccumulatedTime > 0.199 && AccumulatedTime < 0.201, "Ticker engine too imprecise (%f/0.2)", AccumulatedTime );

	unguardtest
}

#endif