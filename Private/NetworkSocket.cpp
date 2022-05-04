/*============================================================================
	Socket.cpp
	Author: Fernando Vel�zquez

	Definitions for platform independant abstractions for Sockets
============================================================================*/


#if _WINDOWS
// WinSock includes.
	#define __WINSOCK__ 1
	#pragma comment(lib,"ws2_32.lib") //Use Winsock2
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <conio.h>
	#define MSG_NOSIGNAL		0
	#undef gai_strerror
	#define gai_strerror gai_strerrorA
#else
// BSD socket includes.
	#define __BSD_SOCKETS__ 1
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/ip.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>
	#include <fcntl.h>

	#ifndef MSG_NOSIGNAL
		#define MSG_NOSIGNAL 0x4000
	#endif
#endif

#include "CacusLibPrivate.h"

#include "CacusThread.h"
#include "CacusString.h"
#include "NetworkSocket.h"
#include "AppTime.h"
#include "Atomics.h"
#include "Math/Math.h"


// Provide WinSock definitions for BSD sockets.
#if _UNIX || _unix
	#define INVALID_SOCKET      -1
	#define SOCKET_ERROR        -1

/*	#define ECONNREFUSED        111
	#define EAGAIN              11*/

	#ifndef SOCKET
		#define SOCKET int
	#endif

	#define closesocket(id) close(id)

#endif



/*----------------------------------------------------------------------------
	Generic socket.
----------------------------------------------------------------------------*/

const int_p SocketGeneric::InvalidSocket = (int_p)INVALID_SOCKET;
const int32 SocketGeneric::Error = SOCKET_ERROR;


static int32 FirstSocket = 0;
SocketGeneric::SocketGeneric( bool bTCP)
{
	SocketGeneric::Init();
	SocketDescriptor = socket
			(
				GIPv6 ? PF_INET6 : PF_INET, //How to open multisocket?
				bTCP ? SOCK_STREAM : SOCK_DGRAM,
				bTCP ? IPPROTO_TCP : IPPROTO_UDP
			);
	LastError = ErrorCode();

	//TODO: Check if opened

	if ( GIPv6 ) //TODO: Check socket status and enable dual-stack
	{
		int Zero = 0;
		LastError = setsockopt( SocketDescriptor, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&Zero, sizeof(Zero));
	}
}

bool SocketGeneric::Init()
{
	if ( !FirstSocket )
	{
		FirstSocket = 1;
		DebugCallback( "Cacus Socket initialized", CACUS_CALLBACK_NET);
	}
	return true;
}

bool SocketGeneric::Close()
{
	if ( SocketDescriptor != INVALID_SOCKET )
	{
		LastError = closesocket(SocketDescriptor);
		SocketDescriptor = INVALID_SOCKET;
		return LastError == 0;
	}
	return false;
}

bool SocketGeneric::Accept( IPEndpoint& Source, CSocket& NewSocket)
{
	uint8 addrbuf[sizeof(sockaddr_in6)]; //Size of sockaddr_6 is 28, this should be safe for both kinds of sockets
	socklen_t addrsize = sizeof(addrbuf);

	SOCKET NewDescriptor = accept( SocketDescriptor, (sockaddr*)addrbuf, &addrsize);
	if ( NewDescriptor == CSocket::InvalidSocket )
	{
		// Handle failure
		int32 NewError = CSocket::ErrorCode();
		if ( !CSocket::IsNonBlocking(NewError) )
		{
			LastError = NewError;
			DebugCallback( CSprintf("Socket::Accept error: %s", CSocket::ErrorText(NewError)), CACUS_CALLBACK_NET);
		}
		return false;
	}
	else
	{
		// Success
		if ( GIPv6 )
			Source = *(sockaddr_in6*)addrbuf;
		else
			Source = *(sockaddr_in*)addrbuf;
		NewSocket.Close();
		NewSocket.SocketDescriptor = NewDescriptor;
		NewSocket.LastError        = 0;
		return true;
	}
}

bool SocketGeneric::Connect( IPEndpoint& RemoteAddress)
{
	if ( GIPv6 )
	{
		sockaddr_in6 addr = RemoteAddress.ToSockAddr6();
		LastError = connect( SocketDescriptor, (sockaddr*)&addr, sizeof(addr));
	}
	else
	{
		sockaddr_in addr = RemoteAddress.ToSockAddr();
		LastError = connect( SocketDescriptor, (sockaddr*)&addr, sizeof(addr));
	}
	if ( LastError )
		LastError = CSocket::ErrorCode();
	return LastError == 0;
}

bool SocketGeneric::Send( const uint8* Buffer, int32 BufferSize, int32& BytesSent)
{
	BytesSent = send( SocketDescriptor, (const char*)Buffer, BufferSize, 0);
	LastError = (BytesSent < 0) ? CSocket::ErrorCode() : 0;
	return BytesSent >= 0;
}

bool SocketGeneric::SendTo( const uint8* Buffer, int32 BufferSize, int32& BytesSent, const IPEndpoint& Dest)
{
	if ( GIPv6 )
	{
		sockaddr_in6 addr = Dest.ToSockAddr6();
		BytesSent = sendto( SocketDescriptor, (const char*)Buffer, BufferSize, 0, (sockaddr*)&addr, sizeof(addr) );
	}
	else
	{
		sockaddr_in addr = Dest.ToSockAddr();
		BytesSent = sendto( SocketDescriptor, (const char*)Buffer, BufferSize, 0, (sockaddr*)&addr, sizeof(addr) );
	}
	LastError = (BytesSent < 0) ? CSocket::ErrorCode() : 0;
	return BytesSent >= 0;
}

bool SocketGeneric::Recv( uint8* Data, int32 BufferSize, int32& BytesRead)
{
	BytesRead = recv( SocketDescriptor, (char*)Data, BufferSize, 0);
	LastError = (BytesRead < 0) ? CSocket::ErrorCode() : 0;
	return BytesRead >= 0;
}

bool SocketGeneric::RecvFrom( uint8* Data, int32 BufferSize, int32& BytesRead, IPEndpoint& Source)
{
	uint8 addrbuf[sizeof(sockaddr_in6)]; //Size of sockaddr_6 is 28, this should be safe for both kinds of sockets
	int32 addrsize = sizeof(addrbuf);

	BytesRead = recvfrom( SocketDescriptor, (char*)Data, BufferSize, 0, (sockaddr*)addrbuf, (socklen_t*)&addrsize);
	if ( GIPv6 )
		Source = *(sockaddr_in6*)addrbuf;
	else
		Source = *(sockaddr_in*)addrbuf;
	LastError = (BytesRead < 0) ? CSocket::ErrorCode() : 0;
	return BytesRead >= 0;
}

bool SocketGeneric::Poll()
{
//	pollfd pfd;
	// TODO: Implement
	return false;
}

bool SocketGeneric::Listen( int32 Backlog)
{
	if ( listen(SocketDescriptor,Backlog) )
	{
		LastError = CSocket::ErrorCode();
		return false;
	}
	else
	{
		LastError = 0;
		return true;
	}
}

bool SocketGeneric::EnableBroadcast( bool bEnable)
{
	int32 Enable = bEnable ? 1 : 0;
	LastError = setsockopt( SocketDescriptor, SOL_SOCKET, SO_BROADCAST, (char*)&Enable, sizeof(Enable));
	return LastError == 0;
}

void SocketGeneric::SetQueueSize( int32 RecvSize, int32 SendSize)
{
	socklen_t BufSize = sizeof(RecvSize);
	setsockopt( SocketDescriptor, SOL_SOCKET, SO_RCVBUF, (char*)&RecvSize, BufSize );
	getsockopt( SocketDescriptor, SOL_SOCKET, SO_RCVBUF, (char*)&RecvSize, &BufSize );
	setsockopt( SocketDescriptor, SOL_SOCKET, SO_SNDBUF, (char*)&SendSize, BufSize );
	getsockopt( SocketDescriptor, SOL_SOCKET, SO_SNDBUF, (char*)&SendSize, &BufSize );
	DebugCallback( CSprintf("%s: Socket queue %i / %i", CSocket::API, RecvSize, SendSize), CACUS_CALLBACK_NET);
}

uint16 SocketGeneric::BindPort( IPEndpoint& LocalAddress, int NumTries, int Increment)
{
	for( int32 i=0 ; i<NumTries ; i++ )
	{
		if ( GIPv6 )
		{
			sockaddr_in6 addr = LocalAddress.ToSockAddr6();
			LastError = bind( SocketDescriptor, (sockaddr*)&addr, sizeof(addr));
			if( !LastError ) //Zero ret = success
			{
				if ( LocalAddress.Port == 0 ) //A random client port was requested, get it
				{
					sockaddr_in6 bound;
					socklen_t size = sizeof(bound);
					LastError = getsockname( SocketDescriptor, (sockaddr*)(&bound), &size);
					LocalAddress.Port = ntohs(bound.sin6_port);
				}
				return LocalAddress.Port;
			}
			else
				DebugCallback( CSprintf( "Failed to bind IPv6 port %i (%s)", (int)LocalAddress.Port, CSocket::ErrorText(LastError)), CACUS_CALLBACK_NET);
		}
		else
		{
			sockaddr_in addr = LocalAddress.ToSockAddr();
			LastError = bind( SocketDescriptor, (sockaddr*)&addr, sizeof(addr));
			if( !LastError ) //Zero ret = success
			{
				if ( LocalAddress.Port == 0 ) //A random client port was requested, get it
				{
					sockaddr_in bound;
					socklen_t size = sizeof(bound);
					LastError = getsockname( SocketDescriptor, (sockaddr*)(&bound), &size);
					LocalAddress.Port = ntohs(bound.sin_port);
				}
				return LocalAddress.Port;
			}
			else
				DebugCallback( CSprintf( "Failed to bind IPv4 port %i (%s)", (int)LocalAddress.Port, CSocket::ErrorText(LastError)), CACUS_CALLBACK_NET);
		}

		if( LocalAddress.Port == 0 ) //Random binding failed/went full circle in port range
			break;
		LocalAddress.Port += (int16)Increment;
	}
	return 0;
}

ESocketState SocketGeneric::CheckState( ESocketState CheckFor, double WaitTime)
{
	fd_set SocketSet;
	timeval Time;

	int Status = 0;
	while ( (WaitTime >= 0) && (Status == 0) )
	{
		double StartTime = FPlatformTime::InitTiming();
		Time.tv_sec = CFloor(WaitTime);
		Time.tv_usec = CFloor((WaitTime - (double)Time.tv_sec) * 1000.0 * 1000.0);
		FD_ZERO(&SocketSet);
		FD_SET( ((SOCKET)SocketDescriptor), &SocketSet);

		int BSDCompat = (int)SocketDescriptor + 1;
		if      ( CheckFor == SOCKET_Readable ) Status = select(BSDCompat, &SocketSet, nullptr, nullptr, &Time);
		else if ( CheckFor == SOCKET_Writable ) Status = select(BSDCompat, nullptr, &SocketSet, nullptr, &Time);
		else if ( CheckFor == SOCKET_HasError ) Status = select(BSDCompat, nullptr, nullptr, &SocketSet, &Time);
		LastError = Status;

		if ( Status == Error )
			return SOCKET_HasError;
		else if ( Status == 0 )
		{
			if ( WaitTime > 0 ) //Do not spin
				Sleep(1);
			WaitTime -= FPlatformTime::Seconds() - StartTime;
			if ( WaitTime <= 0 ) //Timed out
				return SOCKET_Timeout;
		}
	}
	return CheckFor;
}

/*ESocketState SocketGeneric::CheckState( ESocketState CheckFor, double WaitTime)
{
	fd_set SocketSet;
	timeval Time;

	Time.tv_sec = CFloor(WaitTime);
	Time.tv_usec = CFloor((WaitTime - (double)Time.tv_sec) * 1000.0 * 1000.0);
	FD_ZERO(&SocketSet);
	FD_SET( ((SOCKET)SocketDescriptor), &SocketSet);

	int Status = 0;
	int BSDCompat = (int)SocketDescriptor + 1;
	if      ( CheckFor == SOCKET_Readable ) Status = select(BSDCompat, &SocketSet, nullptr, nullptr, &Time);
	else if ( CheckFor == SOCKET_Writable ) Status = select(BSDCompat, nullptr, &SocketSet, nullptr, &Time);
	else if ( CheckFor == SOCKET_HasError ) Status = select(BSDCompat, nullptr, nullptr, &SocketSet, &Time);
	LastError = Status;

	if ( Status == Error )
		return SOCKET_HasError;
	else if ( Status == 0 )
		return SOCKET_Timeout;
	return CheckFor;
}*/

/*----------------------------------------------------------------------------
	Windows socket.
----------------------------------------------------------------------------*/
#ifdef __WINSOCK__

const int32 SocketWindows::EPortUnreach = WSAECONNRESET;
const char* SocketWindows::API = "WinSock";

bool SocketWindows::Init()
{
	SocketGeneric::Init();

	// Init WSA.
	static uint32 Tried = 0;
	if( !Tried )
	{
		Tried = 1;
		WSADATA WSAData;
		int32 Code = WSAStartup( MAKEWORD(2,2), &WSAData );
		if( !Code )
		{
			DebugCallback( CSprintf("WinSock: version %i.%i (%i.%i), MaxSocks=%i, MaxUdp=%i",
				WSAData.wVersion>>8,WSAData.wVersion&255,
				WSAData.wHighVersion>>8,WSAData.wHighVersion&255,
				WSAData.iMaxSockets,WSAData.iMaxUdpDg),
				CACUS_CALLBACK_NET);
		}
		else
		{
			DebugCallback( CSprintf("WSAStartup failed (%s)", ErrorText(Code)), CACUS_CALLBACK_NET);
			return false;
		}
	}
	return true;
}

// This connection will not block the thread, must poll repeatedly to see if it's properly established
bool SocketWindows::SetNonBlocking()
{
	uint32 NoBlock = 1;
	LastError = ioctlsocket( SocketDescriptor, FIONBIO, &NoBlock );
	return LastError == 0;
}

// Reopen connection if a packet arrived after being closed? (apt for servers)
bool SocketWindows::SetReuseAddr( bool bReUse )
{
	char optval = bReUse ? 1 : 0;
	LastError = setsockopt( SocketDescriptor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bool bSuccess = (LastError == 0);
	if ( !bSuccess )
		DebugCallback( "setsockopt with SO_REUSEADDR failed", CACUS_CALLBACK_NET);
	return bSuccess;
}

// This connection will not gracefully shutdown, and will discard all pending data when closed
bool SocketWindows::SetLinger()
{
	linger ling;
	ling.l_onoff  = 1;	// linger on
	ling.l_linger = 0;	// timeout in seconds
	LastError = setsockopt( SocketDescriptor, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));
	return LastError == 0;
}

const char* SocketWindows::ErrorText( int32 Code)
{
	if( Code == -1 )
		Code = WSAGetLastError();
	switch( Code )
	{
	#define CASE(n) case n: return (#n);
		CASE(WSAEINTR)
		CASE(WSAEBADF)
		CASE(WSAEACCES)
		CASE(WSAEFAULT)
		CASE(WSAEINVAL)
		CASE(WSAEMFILE)
		CASE(WSAEWOULDBLOCK)
		CASE(WSAEINPROGRESS)
		CASE(WSAEALREADY)
		CASE(WSAENOTSOCK)
		CASE(WSAEDESTADDRREQ)
		CASE(WSAEMSGSIZE)
		CASE(WSAEPROTOTYPE)
		CASE(WSAENOPROTOOPT)
		CASE(WSAEPROTONOSUPPORT)
		CASE(WSAESOCKTNOSUPPORT)
		CASE(WSAEOPNOTSUPP)
		CASE(WSAEPFNOSUPPORT)
		CASE(WSAEAFNOSUPPORT)
		CASE(WSAEADDRINUSE)
		CASE(WSAEADDRNOTAVAIL)
		CASE(WSAENETDOWN)
		CASE(WSAENETUNREACH)
		CASE(WSAENETRESET)
		CASE(WSAECONNABORTED)
		CASE(WSAECONNRESET)
		CASE(WSAENOBUFS)
		CASE(WSAEISCONN)
		CASE(WSAENOTCONN)
		CASE(WSAESHUTDOWN)
		CASE(WSAETOOMANYREFS)
		CASE(WSAETIMEDOUT)
		CASE(WSAECONNREFUSED)
		CASE(WSAELOOP)
		CASE(WSAENAMETOOLONG)
		CASE(WSAEHOSTDOWN)
		CASE(WSAEHOSTUNREACH)
		CASE(WSAENOTEMPTY)
		CASE(WSAEPROCLIM)
		CASE(WSAEUSERS)
		CASE(WSAEDQUOT)
		CASE(WSAESTALE)
		CASE(WSAEREMOTE)
		CASE(WSAEDISCON)
		CASE(WSASYSNOTREADY)
		CASE(WSAVERNOTSUPPORTED)
		CASE(WSANOTINITIALISED)
		CASE(WSAHOST_NOT_FOUND)
		CASE(WSATRY_AGAIN)
		CASE(WSANO_RECOVERY)
		CASE(WSANO_DATA)
		case 0:						return ("WSANO_ERROR");
		default:					return ("WSA_Unknown");
	#undef CASE
	}
}

inline int32 SocketWindows::ErrorCode()
{
	return WSAGetLastError();
}

inline bool SocketWindows::IsNonBlocking( int32 Code)
{
	return (Code == WSAEWOULDBLOCK) || (Code == WSAEINPROGRESS);
}

ESocketState SocketWindows::CheckState( ESocketState CheckFor, double WaitTime)
{
	fd_set SocketSet;
	timeval Time;

	int Status = 0;
	while ( (WaitTime >= 0) && (Status == 0) )
	{
		double StartTime = FPlatformTime::Seconds();
		Time.tv_sec = CFloor(WaitTime);
		Time.tv_usec = CFloor((WaitTime - (double)Time.tv_sec) * 1000.0 * 1000.0);
		FD_ZERO(&SocketSet);
		FD_SET( ((SOCKET)SocketDescriptor), &SocketSet);

		int BSDCompat = (int)SocketDescriptor + 1;
		if      ( CheckFor == SOCKET_Readable ) Status = select(BSDCompat, &SocketSet, nullptr, nullptr, &Time);
		else if ( CheckFor == SOCKET_Writable ) Status = select(BSDCompat, nullptr, &SocketSet, nullptr, &Time);
		else if ( CheckFor == SOCKET_HasError ) Status = select(BSDCompat, nullptr, nullptr, &SocketSet, &Time);
		LastError = Status;

		if ( Status == Error )
			return SOCKET_HasError;
		else if ( Status == 0 )
		{
			if ( WaitTime > 0 ) //Do not spin
				Sleep(1);
			WaitTime -= FPlatformTime::Seconds() - StartTime;
			if ( WaitTime <= 0 ) //Timed out
				return SOCKET_Timeout;
		}
	}
	return CheckFor;
}

#endif
/*----------------------------------------------------------------------------
	Unix socket.
----------------------------------------------------------------------------*/
#ifdef __BSD_SOCKETS__

const int32 SocketBSD::EPortUnreach = ECONNREFUSED;
const char* SocketBSD::API = ("Sockets");

// This connection will not block the thread, must poll repeatedly to see if it's properly established
bool SocketBSD::SetNonBlocking()
{
	int32 pd_flags;
	pd_flags = fcntl( SocketDescriptor, F_GETFL, 0 );
	pd_flags |= O_NONBLOCK;
	LastError = fcntl( SocketDescriptor, F_SETFL, pd_flags );
	return LastError == 0;
}

// Reopen connection if a packet arrived after being closed? (apt for servers)
bool SocketBSD::SetReuseAddr( bool bReUse )
{
	int32 optval = bReUse ? 1 : 0;
	LastError = setsockopt( SocketDescriptor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	return LastError == 0;
}

// This connection will not gracefully shutdown, and will discard all pending data when closed
bool SocketBSD::SetLinger()
{
	linger ling;
	ling.l_onoff  = 1;	// linger on
	ling.l_linger = 0;	// timeout in seconds
	LastError = setsockopt( SocketDescriptor, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
	return LastError == 0;
}

bool SocketBSD::SetRecvErr()
{
	int32 on = 1;
	LastError = setsockopt(SocketDescriptor, SOL_IP, IP_RECVERR, &on, sizeof(on));
	bool bSuccess = (LastError == 0);
	if ( !bSuccess )
		DebugCallback( ("setsockopt with IP_RECVERR failed"), CACUS_CALLBACK_NET);
	return bSuccess;
}

const char* SocketBSD::ErrorText( int32 Code)
{
	if( Code == -1 )
		Code = errno;
	switch( Code )
	{
	case EINTR:					return ("EINTR");
	case EBADF:					return ("EBADF");
	case EACCES:				return ("EACCES");
	case EFAULT:				return ("EFAULT");
	case EINVAL:				return ("EINVAL");
	case EMFILE:				return ("EMFILE");
	case EWOULDBLOCK:			return ("EWOULDBLOCK");
	case EINPROGRESS:			return ("EINPROGRESS");
	case EALREADY:				return ("EALREADY");
	case ENOTSOCK:				return ("ENOTSOCK");
	case EDESTADDRREQ:			return ("EDESTADDRREQ");
	case EMSGSIZE:				return ("EMSGSIZE");
	case EPROTOTYPE:			return ("EPROTOTYPE");
	case ENOPROTOOPT:			return ("ENOPROTOOPT");
	case EPROTONOSUPPORT:		return ("EPROTONOSUPPORT");
	case ESOCKTNOSUPPORT:		return ("ESOCKTNOSUPPORT");
	case EOPNOTSUPP:			return ("EOPNOTSUPP");
	case EPFNOSUPPORT:			return ("EPFNOSUPPORT");
	case EAFNOSUPPORT:			return ("EAFNOSUPPORT");
	case EADDRINUSE:			return ("EADDRINUSE");
	case EADDRNOTAVAIL:			return ("EADDRNOTAVAIL");
	case ENETDOWN:				return ("ENETDOWN");
	case ENETUNREACH:			return ("ENETUNREACH");
	case ENETRESET:				return ("ENETRESET");
	case ECONNABORTED:			return ("ECONNABORTED");
	case ECONNRESET:			return ("ECONNRESET");
	case ENOBUFS:				return ("ENOBUFS");
	case EISCONN:				return ("EISCONN");
	case ENOTCONN:				return ("ENOTCONN");
	case ESHUTDOWN:				return ("ESHUTDOWN");
	case ETOOMANYREFS:			return ("ETOOMANYREFS");
	case ETIMEDOUT:				return ("ETIMEDOUT");
	case ECONNREFUSED:			return ("ECONNREFUSED");
	case ELOOP:					return ("ELOOP");
	case ENAMETOOLONG:			return ("ENAMETOOLONG");
	case EHOSTDOWN:				return ("EHOSTDOWN");
	case EHOSTUNREACH:			return ("EHOSTUNREACH");
	case ENOTEMPTY:				return ("ENOTEMPTY");
	case EUSERS:				return ("EUSERS");
	case EDQUOT:				return ("EDQUOT");
	case ESTALE:				return ("ESTALE");
	case EREMOTE:				return ("EREMOTE");
	case HOST_NOT_FOUND:		return ("HOST_NOT_FOUND");
	case TRY_AGAIN:				return ("TRY_AGAIN");
	case NO_RECOVERY:			return ("NO_RECOVERY");
	case 0:						return ("NO_ERROR");
	default:					return ("Unknown");
	}
}

int32 SocketBSD::ErrorCode()
{
	return errno;
}

bool SocketBSD::IsNonBlocking( int32 Code)
{
	return (Code == EAGAIN) || (Code == EINPROGRESS) || (Code == EALREADY);
}

#endif

/*----------------------------------------------------------------------------
	Other.
----------------------------------------------------------------------------*/

const char* SocketGeneric::GetHostname()
{
	TChar8Buffer<257> LocalBuffer;
	if ( gethostname( *LocalBuffer, (int)LocalBuffer.Size()) )
		return "";
	return CopyToBuffer(*LocalBuffer);
}

IPAddress SocketGeneric::ResolveHostname( const char* HostName, bool bOnlyParse, bool bCallbackException)
{
	if ( !_stricmp(HostName,"all") || !_stricmp(HostName,"any") )
		return IPAddress::Any;

	addrinfo Hint, Hint4, *Result;
	memset( &Hint, 0, sizeof(Hint));
	Hint.ai_family = GIPv6 ? AF_INET6 : AF_INET; //Get IPv4 or IPv6 addresses
	if ( GIPv6 )
		Hint.ai_flags |= AI_V4MAPPED | AI_ADDRCONFIG;
	if ( bOnlyParse )
		Hint.ai_flags |= AI_NUMERICHOST;

	if ( GIPv6 && bOnlyParse ) //IPv4 cannot be properly parsed with AF_INET6
	{
		memset( &Hint4, 0, sizeof(Hint4) );
		Hint4.ai_family = AF_INET;
		Hint4.ai_flags = AI_NUMERICHOST;
		Hint.ai_next = &Hint4;
	}

	IPAddress Address = IPAddress::Any;
	const uint32 CACUS_CALLBACK_FLAGS = CACUS_CALLBACK_NET | (bCallbackException ? CACUS_CALLBACK_EXCEPTION : 0);

	//Purposely failing to pass HostName will trigger 'gethostname' (useful for init)
	const char* LocalHostName = nullptr;
	if ( *HostName == '\0' )
	{
		LocalHostName = GetHostname();
		if ( *LocalHostName == '\0' )
		{
			DebugCallback( CSprintf( "gethostname failed (%s)", CSocket::ErrorText()), CACUS_CALLBACK_FLAGS);
			return Address;
		}
		HostName = LocalHostName;
		DebugCallback( CSprintf("Local hostname: %s", LocalHostName), CACUS_CALLBACK_NET);
	}

	int32 ErrorCode = getaddrinfo( HostName, NULL, &Hint, &Result);
	if ( ErrorCode != 0 )
	{
		DebugCallback( CSprintf( "getaddrinfo failed %s: %s", HostName, gai_strerror(ErrorCode)), CACUS_CALLBACK_FLAGS);
	}
	else
	{
		if ( GIPv6 ) //Prioritize IPv6 result
		{
			for ( addrinfo* Link=Result ; Link ; Link=Link->ai_next )
				if ( Link->ai_family==AF_INET6 && Link->ai_addr )
				{
					in6_addr* addr = &((sockaddr_in6*)Link->ai_addr)->sin6_addr; //IPv6 struct
					if ( IN6_IS_ADDR_LINKLOCAL( addr) )
						continue;
					Address = *addr; 
					if ( Address != IPAddress::Any )
						return Address;
				}
		}

		for ( addrinfo* Link=Result ; Link ; Link=Link->ai_next )
			if ( (Link->ai_family==AF_INET) && Link->ai_addr )
			{
				Address = ((sockaddr_in*)Link->ai_addr)->sin_addr; //IPv4 struct
				if ( Address != IPAddress::Any )
					return Address;
			}
		DebugCallback( CSprintf( "Unable to find host %s", HostName), CACUS_CALLBACK_FLAGS);
	}
	return Address;
}


/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/

