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

#include "Parser/Line.h"

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
			printf("==PACKET START\n%s\n==PACKET END\n",Buf);
		}
	}
	Socket.Close();
}

//
// Simple threaded session, contained here.
//
struct UpnpRouterSession
{

};


const char* DiscoverRootDesc()
{
	const char* Result = nullptr;

	// Init socket
	int32 BytesSent = 0;
	CSocket::Init();
	IPEndpoint LocalEndpoint( IPAddress(0,0,0,0), 8060 );
	LocalEndpoint.Address = CSocket::ResolveHostname("", false);

	// Multicast emit
	CSocket Socket(false);
	Socket.BindPort(LocalEndpoint);
	Socket.SetNonBlocking();

	size_t SendCount = 0;
	{
		// TODO: try multiple SSDP addresses?
		const IPEndpoint& EndPoint = IPEndpoint::SSDPMulticast_v4;
		const char* Request = CSprintf(
			"M-SEARCH * HTTP/1.1"   "\r\n"
			"HOST:%s"               "\r\n"
			"MAN:\"ssdp:discover\"" "\r\n"
			"MX:2"                  "\r\n"
			"ST:upnp:rootdevice"    "\r\n"
			"\r\n"
			, *EndPoint);

		bool Sent = Socket.SendTo( (uint8*)Request, CStrlen(Request), BytesSent, EndPoint);
		if ( Sent )
		{
			SendCount += (size_t)Sent;
		}
	}

	double Start = FPlatformTime::InitTiming();
	double Current;
	while ( (Current=FPlatformTime::Seconds()) - Start < 2.0 )
	{
		Sleep(1);
		uint8 Buf[4097];
		int32 Read = 0;
		if ( Socket.Recv(Buf,4096,Read) )
		{
			Buf[Read] = 0;
			// TODO: PARSE UTF-8!!
			CParserFastLine8 LineParser;
			if ( LineParser.Parse((const char*)Buf) )
			{
				const char* Location = nullptr;
				for ( const char** LineArray=LineParser.GetLineArray(); *LineArray; LineArray++)
				{
					printf("%s\n", *LineArray);
					if ( !CStrnicmp(*LineArray,"Location: ") )
						Location = *LineArray + _len("Location: ");
				}
				if ( Location )
					Result = CopyToBuffer(Location);
			}
		}
	}
	Socket.Close();

	return Result;
}



bool SetUpnpPort( int Port, bool Enable)
{
	const char* RootDesc = DiscoverRootDesc();
	if ( !RootDesc )
		return false;

	printf("RootDesc found at: %s\n", RootDesc);

	return false;
}
