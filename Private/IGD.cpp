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
#include "CacusMem.h"

#include "Parser/Line.h"
#include "Parser/UTF.h"

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

	if ( !Socket.SendTo( (uint8*)Request, (int32)CStrlen(Request), BytesSent, IPEndpoint::SSDPMulticast_v4) )
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
// Simple session, contained in small functions here.
//
struct UpnpRouterSession
{
	IPAddress LocalAddress;
	CParserFastLine8 SSDP_Response;
	URI RootDescURI;
	IPEndpoint RootDescEndpoint;
	CParserUTF RootDescFile;

	bool ResolveLocalAddress();
	bool GetDeviceRootDescLocation();
	bool ResolveRootDescAddress();
	bool GetDeviceRootDescFile();
};


bool UpnpRouterSession::ResolveLocalAddress()
{
	CSocket::Init();
	LocalAddress = CSocket::ResolveHostname("", false);
	return LocalAddress != IPAddress::Any;
}


bool UpnpRouterSession::GetDeviceRootDescLocation()
{
	// Init socket
	bool Result = false;
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

		bool Sent = Socket.SendTo( (uint8*)Request, (int32)CStrlen(Request), BytesSent, EndPoint);
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
						RootDescURI = URI(*LineArray + _len("Location: "));
				}
				// Appears to be a valid URI
				if ( !CStricmp(RootDescURI.Scheme(),"http") && RootDescURI.Hostname() )
				{
					Result = true;
					break;
				}
			}
		}
	}
	Socket.Close();

	return Result;
}

bool UpnpRouterSession::ResolveRootDescAddress()
{
	RootDescEndpoint.Address = CSocket::ResolveHostname(RootDescURI.Hostname());
	RootDescEndpoint.Port    = RootDescURI.Port();
	return RootDescEndpoint.Address != IPAddress::Any;
}

bool UpnpRouterSession::GetDeviceRootDescFile()
{
	CSocket Socket(true);
	Socket.SetNonBlocking();
	Socket.Connect(RootDescEndpoint);
	if ( Socket.CheckState(SOCKET_Writable, 1.0) != SOCKET_Writable )
	{
		Socket.Close();
		return false;
	}


	const char* Request = CSprintf(
	"GET %s HTTP/1.1"                   "\r\n"
	"Host: %s"                          "\r\n"
	"User-Agent: CacusLib IGD"          "\r\n"
	"Accept: application/xml,text/xml"  "\r\n"
	"Connection: close"                 "\r\n"
	"\r\n",
		RootDescURI.Path(),
		*RootDescEndpoint);

	int32 Sent = 0;
	Socket.Send((uint8*)Request, (int32)CStrlen(Request), Sent);

	CScopeMem DownloadTotal;
	int32     DownloadPos   = 0;
	int32     ContentLength = 0;
	double    LastRecv      = FPlatformTime::InitTiming();
	{
		int32 Read         = 0;
		int32 TotalRead    = 0;
		int32 ContentStart = 0;

		CParserFastLine8 HeaderParser;

		static constexpr int BufferSize = 2048;
		uint8 InitialBuffer[BufferSize+EALIGN_PLATFORM_PTR];

		while ( !ContentStart && (TotalRead < BufferSize) )
		{
			if ( Socket.Recv(&InitialBuffer[TotalRead], BufferSize-TotalRead, Read) )
			{
				printf("Recv %i\n", Read);
				LastRecv = FPlatformTime::Seconds();
				if ( Read == 0 ) //Shutdown
					break;

				TotalRead += Read;
				InitialBuffer[TotalRead] = 0;
				
				// Attempt to process header everytime data is received
				uint8* EmptyRN = (uint8*)CStrstr((char*)InitialBuffer, "\r\n\r\n");
				uint8* EmptyN  = (uint8*)CStrstr((char*)InitialBuffer, "\n\n");
				if ( EmptyRN && (!EmptyN || EmptyN>EmptyRN) )
				{
					ContentStart = (int32)(EmptyRN - InitialBuffer) + 4;
					EmptyRN[2] = '\0';
					EmptyRN[3] = '\0';
				}
				if ( EmptyN && (!EmptyRN || EmptyRN>EmptyN) )
				{
					ContentStart = (int32)(EmptyN - InitialBuffer) + 2;
					EmptyN[1] = '\0';
				}

				// Parse lines up to empty line
				if ( ContentStart && HeaderParser.Parse((char*)InitialBuffer) )
				{
					for ( const char** Line=HeaderParser.GetLineArray(); *Line; Line++)
						if ( !CStrnicmp(*Line,"Content-Length: ") )
						{
							ContentLength = atoi(*Line + _len("Content-Length: "));
							break;
						}
				}
			}

			// No need to wait if header was succesfully received and parsed
			if ( !ContentLength )
			{
				// Timeout
				if ( FPlatformTime::Seconds()-LastRecv > 1.0 )
					break;
				Sleep(5);
			}
		}

		// No/bad response.
		if ( !ContentStart || !ContentLength )
		{
			Socket.Close();
			return false;
		}

//		printf("ContentLength is %i\n", ContentLength);
//		printf("ContentStart is %i\n", ContentStart);

		// Setup download chunk
		DownloadTotal = CScopeMem((size_t)ContentLength+1);
		if ( ContentStart < TotalRead )
		{
			DownloadPos = TotalRead - ContentStart;
			CMemcpy(DownloadTotal.GetData(), &InitialBuffer[ContentStart], DownloadPos);
		}

		// Fill the download buffer now
		while ( DownloadPos < ContentLength )
		{
			if ( Socket.Recv(DownloadTotal.GetArray<uint8>()+DownloadPos, ContentLength-DownloadPos, Read) )
			{
				printf("Recv %i\n", Read);
				LastRecv = FPlatformTime::Seconds();
				if ( Read == 0 ) //Shutdown
					break;

				DownloadPos += Read;
			}
			else
			{
				// Timeout check
				if ( FPlatformTime::Seconds()-LastRecv > 1.0 )
					break;
				Sleep(5);
			}
		}
		DownloadTotal.GetArray<uint8>()[ContentLength] = 0; //Null terminator
//		printf("Received file of size: %i\n", DownloadPos);
		Socket.Close();

		// File incomplete
		if ( DownloadPos < ContentLength )
			return false;

		// File cannot be decoded
		if ( !RootDescFile.Parse(DownloadTotal.GetData()) )
			return false;
	}

	return true;
}


bool SetUpnpPort( int Port, bool Enable)
{
	UpnpRouterSession Session;

	if ( !Session.ResolveLocalAddress() )
	{
		DebugCallback("Unable to resolve local network address", CACUS_CALLBACK_TEST);
		return false;
	}

	if ( !Session.GetDeviceRootDescLocation() )
	{
		DebugCallback("Unable to find UPnP Root device descriptor", CACUS_CALLBACK_TEST);
		return false;
	}

	if ( !Session.ResolveRootDescAddress() )
	{
		DebugCallback(CSprintf("Unable to resolve host %s", Session.RootDescURI.Hostname()), CACUS_CALLBACK_TEST);
		return false;
	}

	if ( !Session.GetDeviceRootDescFile() )
	{
		DebugCallback("Unable to retrieve UPnP Root device descriptor", CACUS_CALLBACK_TEST);
		return false;
	}

	wprintf(L"RootDesc: \n%s\n", Session.RootDescFile.Output);
	


	return false;
}
