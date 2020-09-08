/*=============================================================================
	NetworkSocket.h:

	Socket instance definition.

	Author: Fernando Vel�zquez.
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

#ifdef _WINDOWS
class SocketWindows;
typedef SocketWindows CSocket;
#else
class SocketBSD;
typedef SocketBSD CSocket;
#endif


class CACUS_API SocketGeneric
{
protected:
	int_p SocketDescriptor;
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
	static bool IsNonBlocking( int32 Code)  {return false;}

	bool Close()                            {SetInvalid(); return false;}
	bool IsInvalid()                        {return SocketDescriptor==InvalidSocket;}
	void SetInvalid()                       {SocketDescriptor=InvalidSocket;}
	bool SetNonBlocking()                   {return true;}
	bool SetReuseAddr( bool bReUse=true)    {return true;}
	bool SetLinger()                        {return true;}
	bool SetRecvErr()                       {return false;}

	bool Accept( IPEndpoint& Source, CSocket& NewSocket);
	bool Connect( IPEndpoint& RemoteAddress);
	bool Send( const uint8* Buffer, int32 BufferSize, int32& BytesSent);
	bool SendTo( const uint8* Buffer, int32 BufferSize, int32& BytesSent, const IPEndpoint& Dest);
	bool Recv( uint8* Data, int32 BufferSize, int32& BytesRead); //Implement flags later
	bool RecvFrom( uint8* Data, int32 BufferSize, int32& BytesRead, IPEndpoint& Source); //Implement flags later, add IPv6 type support
	bool Poll();
	bool Listen( int32 Backlog);
	bool EnableBroadcast( bool bEnable=1);
	void SetQueueSize( int32 RecvSize, int32 SendSize);
	uint16 BindPort( IPEndpoint& LocalAddress, int NumTries=1, int Increment=1);
	ESocketState CheckState( ESocketState CheckFor, double WaitTime=0);

	// Prints to circular buffer
	static const char* GetHostname();

	// Use "all", "any" for IPAddress::Any
	// Use "" to select primary network adapter
	static IPAddress ResolveHostname( const char* HostName, bool bOnlyParse=false, bool bCallbackException=false);
};

#ifdef _WINDOWS
class CACUS_API SocketWindows : public SocketGeneric
{
public:
	static const int32 EPortUnreach;
	static const char* API;

	using SocketGeneric::SocketGeneric;

	static bool Init();
	static const char* ErrorText( int32 Code=-1);
	static int32 ErrorCode();
	static bool IsNonBlocking( int32 Code);

	bool Close();
	bool SetNonBlocking();
	bool SetReuseAddr( bool bReUse=true);
	bool SetLinger();

	ESocketState CheckState( ESocketState CheckFor, double WaitTime=0);
};


#else
class CACUS_API SocketBSD : public SocketGeneric
{
public:
	static const int32 EPortUnreach;
	static const char* API;

	using SocketGeneric::SocketGeneric;

	static const char* ErrorText( int32 Code=-1);
	static int32 ErrorCode();
	static bool IsNonBlocking( int32 Code);

	bool Close();
	bool SetNonBlocking();
	bool SetReuseAddr( bool bReUse=true);
	bool SetLinger();
	bool SetRecvErr();
};


#endif





#endif
