

#include "CacusBase.h"
#include "IGD_TEST.h"
#include "DebugCallback.h"
#include "stdio.h"


void PrintfCallback( const char* Message, int MessageFlags)
{
	printf("%s\n",Message);
}

int main()
{
	CDbg_RegisterCallback( &PrintfCallback, CACUS_CALLBACK_ALL);
//	GetSSDP();
	SetUpnpPort(7777,true);
	CDbg_UnregisterCallback( &PrintfCallback);
}

