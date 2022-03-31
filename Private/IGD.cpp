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
#include "URI.h"

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
	IPAddress LocalAddress;
	CParserFastLine8 SSDP_Response;
	const char* RootDesc_Location;

	bool ResolveLocalAddress();
	bool GetDeviceRootDesc();
};


bool UpnpRouterSession::ResolveLocalAddress()
{
	CSocket::Init();
	LocalAddress = CSocket::ResolveHostname("", false);
	return LocalAddress != IPAddress::Any;
}


bool UpnpRouterSession::GetDeviceRootDesc()
{
	// Init socket
	int32 BytesSent = 0;
	IPEndpoint LocalEndpoint( LocalAddress, 8060 );

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
			if ( SSDP_Response.Parse((const char*)Buf) )
			{
				for ( const char** LineArray=SSDP_Response.GetLineArray(); *LineArray; LineArray++)
				{
					printf("%s\n", *LineArray);
					if ( !CStrnicmp(*LineArray,"Location: ") )
						RootDesc_Location = *LineArray + _len("Location: ");
				}
				if ( RootDesc_Location )
					break;
			}
		}
	}
	Socket.Close();

	return RootDesc_Location != nullptr;
}



bool SetUpnpPort( int Port, bool Enable)
{
	UpnpRouterSession Session;

	if ( !Session.ResolveLocalAddress() )
	{
		DebugCallback("Unable to resolve local network address", CACUS_CALLBACK_TEST);
		return false;
	}

	if ( !Session.GetDeviceRootDesc() )
	{
		DebugCallback("Unable to find UPnP Root device", CACUS_CALLBACK_TEST);
		return false;
	}

	printf("RootDesc found at: %s\n", Session.RootDesc_Location);
	URI RootDesc_URI(Session.RootDesc_Location);
	if ( CStricmp(RootDesc_URI.Scheme(),"http") || !RootDesc_URI.Hostname() || !RootDesc_URI.Path() )
	{
		const char* Message = CSprintf("Invalid Root device descriptor URI: %s", Session.RootDesc_Location);
		DebugCallback(Message, CACUS_CALLBACK_TEST);
		return false;


	return false;
}
