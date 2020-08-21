/*=============================================================================
	NetworkSocket.h:

	Socket instance definition.

	Author: Fernando Velázquez.
=============================================================================*/
#ifndef USES_CACUS_NETWORK_SOCKET
#define USES_CACUS_NETWORK_SOCKET

#include "CacusPlatform.h"
#include "IPv6.h"

/*----------------------------------------------------------------------------
	Unified socket system 
----------------------------------------------------------------------------*/

enum ESocketState
{
	SOCKET_Timeout, //Used for return values
	SOCKET_Readable,
	SOCKET_Writable,
	SOCKET_HasError,
	SOCKET_MAX
};

/*----------------------------------------------------------------------------
	Socket abstraction (win32/unix).
----------------------------------------------------------------------------*/

class CACUS_API SocketGeneric
{
protected:
	int_p Socket;
public:
	int32 LastError;
public:
	static const int_p InvalidSocket;
	static const int32 Error;

	SocketGeneric();
	SocketGeneric( bool bTCP);

	static bool Init();
	static const char* ErrorText( int32 Code=-1)     {return "";}
	static int32 ErrorCode()                {return 0;}

	bool Close()                            {SetInvalid(); return false;}
	bool IsInvalid()                        {return Socket==InvalidSocket;}
	void SetInvalid()                       {Socket=InvalidSocket;}
	bool SetNonBlocking()                   {return true;}
	bool SetReuseAddr( bool bReUse=true)    {return true;}
	bool SetLinger()                        {return true;}
	bool SetRecvErr()                       {return false;}

	bool Connect( IPEndpoint& RemoteAddress);
	bool Send( const uint8* Buffer, int32 BufferSize, int32& BytesSent);
	bool SendTo( const uint8* Buffer, int32 BufferSize, int32& BytesSent, const IPEndpoint& Dest);
	bool Recv( uint8* Data, int32 BufferSize, int32& BytesRead); //Implement flags later
	bool RecvFrom( uint8* Data, int32 BufferSize, int32& BytesRead, IPEndpoint& Source); //Implement flags later, add IPv6 type support
	bool EnableBroadcast( bool bEnable=1);
	void SetQueueSize( int32 RecvSize, int32 SendSize);
	uint16 BindPort( IPEndpoint& LocalAddress, int NumTries=1, int Increment=1);
	ESocketState CheckState( ESocketState CheckFor, double WaitTime=0);

	// Passing an empty HostName fills the value with local address.
	// Return value defaults to IPAddress::Any
	static IPAddress ResolveHostname( char* HostName, size_t HostNameSize, bool bOnlyParse, bool bCallbackException=false);
};

#ifdef _WINDOWS
class CACUS_API SocketWindows : public SocketGeneric
{
public:
	static const int32 ENonBlocking;
	static const int32 EPortUnreach;
	static const char* API;

	SocketWindows() {}
	SocketWindows( bool bTCP) : SocketGeneric(bTCP) {}

	static bool Init();
	static const char* ErrorText( int32 Code=-1);
	static int32 ErrorCode();
	
	bool Close();
	bool SetNonBlocking();
	bool SetReuseAddr( bool bReUse=true);
	bool SetLinger();

	ESocketState CheckState( ESocketState CheckFor, double WaitTime=0);
};
typedef SocketWindows Socket;


#else
class CACUS_API SocketBSD : public SocketGeneric
{
public:
	static const int32 ENonBlocking;
	static const int32 EPortUnreach;
	static const char* API;

	SocketBSD() {}
	SocketBSD( bool bTCP) : SocketGeneric(bTCP) {}

	static const char* ErrorText( int32 Code=-1);
	static int32 ErrorCode();

	bool Close();
	bool SetNonBlocking();
	bool SetReuseAddr( bool bReUse=true);
	bool SetLinger();
	bool SetRecvErr();
};
typedef SocketBSD Socket;


#endif





#endif
