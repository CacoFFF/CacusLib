/*=============================================================================
	CacusHTTP.cpp:
	Simple HTTP browser.
	Author: Fernando Velázquez
=============================================================================*/


#include "../CacusLibPrivate.h"
#include "NetUtils/CacusHTTP.h"

#include "NetworkSocket.h"
#include "CacusString.h"
#include "CacusMem.h"
#include "AppTime.h"
#include "Parser/Line.h"

// Move semantics
#include <utility>




CacusHTTP::CacusHTTP()
	: Timeout(5.0)
	, ContentLength(0)
{
}

//========= ParseResponse() - begin ==========//
//
// Private functionality of CacusHTTP browser.
// 
// (char*) Headers       - Null-terminated header text.
// (bool)  Return value  - Header parsed and valid.
//
static bool ParseResponse( CacusHTTP& Browser, const char* Headers)
{
	CParserFastLine8 HeaderLines;

	// Parse lines up to empty line
	if ( HeaderLines.Parse(Headers) )
	{
		Browser.ContentLength = 0;

		for ( const char** Line=HeaderLines.GetLineArray(); *Line; Line++)
		{
			if ( !Browser.ContentLength && !CStrnicmp(*Line,"Content-Length: ") )
				Browser.ContentLength = atoi(*Line + _len("Content-Length: "));
		}
		return (Browser.ContentLength > 0);
	}
	return false;
}
//========= ParseResponse() - end ==========//


//========= SplitContent() - begin ==========//
//
// Finds the empty line and replaces the second line terminator
// with null terminators.
// 
// (char*) Buffer        - Raw buffer.
// (int32) Offset        - Optional search offset.
// (int32) Return value  - Position of Content.
//
static int32 SplitContent( const char* Buffer, int32 Offset)
{
	const char* SearchStart = Buffer + Offset;

	// Attempt to process header everytime data is received
	char* EmptyRN = CStrstr(Buffer, "\r\n\r\n");
	char* EmptyN  = CStrstr(Buffer, "\n\n");

	if ( EmptyRN && (!EmptyN || EmptyN>EmptyRN) )
	{
		EmptyRN[2] = '\0';
		EmptyRN[3] = '\0';
		return (int32)(EmptyRN - Buffer) + 4;
	}

	if ( EmptyN && (!EmptyRN || EmptyRN>EmptyN) )
	{
		EmptyN[1] = '\0';
		return (int32)(EmptyN - Buffer) + 2;
	}

	return 0;
}
//========= SplitContent() - end ==========//


bool CacusHTTP::Browse( const char* BrowseURI)
{
	// Once URI is validated, object state will be overwritten
	{
		URI NewRequestURI(RequestURI, BrowseURI);
		if ( *NewRequestURI.Hostname() == '\0')
			return false;

		RequestURI = std::move(NewRequestURI);
		ResponseHeaderData.Empty();
		ResponseData.Empty();
		ContentLength = 0;
	}

	// Create the TCP socket and connect to remote host
	CSocket::Init();
	CSocket Socket(true);
	{
		IPEndpoint RemoteEndpoint;
		RemoteEndpoint.Address = CSocket::ResolveHostname( RequestURI.Hostname() );
		RemoteEndpoint.Port    = RequestURI.Port();
		if ( RemoteEndpoint.Address == IPAddress::Any )
			return false;
		if ( RequestURI.Port() == 0 )
			RemoteEndpoint.Port = 80;

		Socket.SetNonBlocking();
		Socket.Connect(RemoteEndpoint);
		if ( Socket.CheckState(SOCKET_Writable, 1.0) != SOCKET_Writable )
			return false;
	}


	const char* Request = CSprintf(
		"GET %s HTTP/1.1"                   "\r\n"
		"Host: %s"                          "\r\n"
		"User-Agent: CacusLib simple HTTP"  "\r\n"
		"Accept: application/xml,text/xml"  "\r\n"
		"Connection: close"                 "\r\n"
		"\r\n",
		RequestURI.Path(),
		RequestURI.Authority());

	int32 Sent = 0;
	Socket.Send((uint8*)Request, (int32)CStrlen(Request), Sent);

	double LastRecv = FPlatformTime::InitTiming();

	{
		int32 Read         = 0;
		int32 TotalRead    = 0;
		int32 ContentStart = 0;

		static constexpr int BufferSize = 2048;
		ResponseHeaderData.Resize(BufferSize + EALIGN_PLATFORM_PTR);
		uint8* Buffer = ResponseHeaderData.GetArray<uint8>();

		while ( !ContentStart && (TotalRead < BufferSize) )
		{
			if ( Socket.Recv( Buffer+TotalRead, BufferSize-TotalRead, Read) )
			{
				LastRecv = FPlatformTime::Seconds();
				if ( Read == 0 ) //Shutdown
					break;

				TotalRead += Read;
				Buffer[TotalRead] = 0;

				if ( (ContentStart=SplitContent((char*)Buffer,0)) != 0 )
				{
					// Here the double end line has already been found
					// Meaning that failure to parse the header is fatal.
					if ( ParseResponse(*this, (char*)Buffer) )
						break;
					return false;
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
			return false;

		// Setup download chunk
		// Additionally move split Header content into the Response buffer
		ResponseData.Resize((size_t)ContentLength + EALIGN_PLATFORM_PTR);
		TotalRead -= ContentStart;
		if ( TotalRead > 0)
			CMemcpy(ResponseData.GetData(), &Buffer[ContentStart], TotalRead);

		// Fill the download buffer now
		while ( TotalRead < ContentLength )
		{
			if ( Socket.Recv(ResponseData.GetArray<uint8>() + TotalRead, ContentLength-TotalRead, Read) )
			{
				LastRecv = FPlatformTime::Seconds();
				if ( Read == 0 ) //Shutdown
					break;
				TotalRead += Read;
			}
			else
			{
				// Timeout check
				if ( FPlatformTime::Seconds()-LastRecv > 1.0 )
					break;
				Sleep(5);
			}
		}
		
		// Zero terminate data (in case we need to use string funcs)
		for ( size_t B=0; B<EALIGN_PLATFORM_PTR; B++)
			ResponseData.GetArray<uint8>()[TotalRead+B] = 0; 

		// Incomplete
		if ( TotalRead < ContentLength )
			return false;
	}

	return true;
}
