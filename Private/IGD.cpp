/*=============================================================================
	IGD.cpp
	Author: Fernando Velázquez
=============================================================================*/


/*
Development notes:

https://datatracker.ietf.org/doc/html/rfc6970



*/

#include "CacusLibPrivate.h"

#include "CacusString.h"
#include "NetworkSocket.h"
#include "AppTime.h"
#include "Atomics.h"

#include "IGD_TEST.h"
#include <stdio.h>

#include <exception>


void GetSSDP()
{
	const char* Request = CSprintf(
"M-SEARCH * HTTP/1.1"   "\r\n"
"HOST:%s"               "\r\n"
"MAN:\"ssdp:discover\"" "\r\n"
"MX:2"                  "\r\n"
"ST:ssdp:all"           "\r\n"
"\r\n"
, *IPEndpoint::SSDPMulticast_v4);

	// Init socket
	int32 BytesSent = 0;
	CSocket::Init();
	IPEndpoint LocalEndpoint( IPAddress(0,0,0,0), 8060 );
	LocalEndpoint.Address = CSocket::ResolveHostname("", false);

	// Multicast emit
	CSocket Socket(false);
	Socket.BindPort(LocalEndpoint);
	Socket.SetNonBlocking();
	printf("== Bound to %s\n", *LocalEndpoint);

	if ( !Socket.SendTo( (uint8*)Request, CStrlen(Request), BytesSent, IPEndpoint::SSDPMulticast_v4) )
		return;
	printf("== Sent request:\n%s\n", Request);

	double Start = FPlatformTime::InitTiming();
	double Current;
	while ( (Current=FPlatformTime::Seconds()) - Start < 2.0 )
	{
		Sleep(1);
		uint8 Buf[1025];
		int32 Read = 0;
		if ( Socket.Recv(Buf,1024,Read) )
		{
			Buf[Read] = 0;
			printf("\n%s\n",Buf);
		}
	}
	Socket.Close();
}
