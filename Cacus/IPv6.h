/*============================================================================
	IPv6.h

	IPv6/IPv4 address container.
	
	Author: Fernando Velázquez
============================================================================*/

#ifndef IPV6_H
#define IPV6_H

#include "DebugCallback.h"

extern "C"
{
	extern CACUS_API bool GIPv6;
};

//Data in host byte order!
class IPAddress
{
	union
	{
		uint8  Bytes[16];
		uint16 Words[8];
		uint32 DWords[4]; //xmmword
		uint64 QWords[2];
	};

public:
	CACUS_API static const IPAddress Any; //::
	CACUS_API static const IPAddress InternalLoopback; //::1
	CACUS_API static const IPAddress InternalLoopback_v4; //::127.0.0.1
	CACUS_API static const IPAddress LanBroadcast; //FF02::1
	CACUS_API static const IPAddress LanBroadcast_v4; //255.255.255.255

public:
	IPAddress()
	:	QWords{0,0}
	{}

	//IPv4 mapped address constructor (::FFFF:A.B.C.D)
	IPAddress( uint8 A, uint8 B, uint8 C, uint8 D)
	{
		Bytes[0]=D;  Bytes[1]=C;  Bytes[2]=B;  Bytes[3]=A;
		Words[2] = 0xFFFF;  Words[3] = 0x0000;
		QWords[1] = 0x0000000000000000;
	}

	//IPv6 address constructor
	IPAddress( uint16 A, uint16 B, uint16 C, uint16 D, uint16 E, uint16 F, uint16 G, uint16 H)
	{
		Words[0]=H;  Words[1]=G;
		Words[2]=F;  Words[3]=E;
		Words[4]=D;  Words[5]=C;
		Words[6]=B;  Words[7]=A;
	}

#ifdef AF_INET
	IPAddress( in_addr addr )
	{
		DWords[0] = ntohl(addr.s_addr);
		Words[2] = 0xFFFF;  Words[3] = 0x0000;
		QWords[1] = 0x0000000000000000;
	}

	inline sockaddr_in ToSockAddr() const
	{
		sockaddr_in Result;
		Result.sin_family = AF_INET;
		Result.sin_port = 0;
		Result.sin_addr.s_addr = htonl(DWords[0]);
		*(uint64*)Result.sin_zero = 0;
		return Result;
	}
#endif
#ifdef AF_INET6
	IPAddress( in6_addr addr )
	{
		//Windows and linux use different union/variable names!!!
		for ( int32 i=0 ; i<16 ; i++ )
			Bytes[i] = addr.s6_addr[15-i];
	}

	inline sockaddr_in6 ToSockAddr6() const
	{
		sockaddr_in6 Result;
		Result.sin6_family = AF_INET6;
		Result.sin6_port = 0;
		//Windows and linux use different union/variable names!!!
		for ( int32 i=0 ; i<16 ; i++ )
			Result.sin6_addr.s6_addr[i] = Bytes[15-i];
		Result.sin6_scope_id = 0;
		return Result;
	}
#endif


	bool operator==( const IPAddress& Other ) const
	{
		return QWords[0] == Other.QWords[0]
			&& QWords[1] == Other.QWords[1];
	}

	bool operator!=( const IPAddress& Other ) const
	{
		return QWords[0] != Other.QWords[0]
			|| QWords[1] != Other.QWords[1];
	}

	bool IsIPv4() const
	{
		return Words[2]==0xFFFF && Words[3]==0x0000 && QWords[1]==0x0000000000000000;
	}

	// Exports to text using the internal string buffer (does not require deallocation)
	CACUS_API const char* operator*() const;

#ifdef CORE_API
	// Unreal Tournament
	friend FORCEINLINE FArchive& operator<<( FArchive& Ar, IPAddress& IpAddr )
	{
		return Ar << (INT&)IpAddr.DWords[0] << (INT&)IpAddr.DWords[1] << (INT&)IpAddr.DWords[2] << (INT&)IpAddr.DWords[3];
	}
#endif

public:
	//Ip in array order: 3.2.1.0
	uint8 GetByte( int32 Index ) const
	{
//		check((Index >= 0) && (Index < 16));
		return Bytes[Index];
	}

	uint32 GetWord( int32 Index=0 ) const
	{
//		check((Index >= 0) && (Index < 8));
		return Words[Index];
	}

	uint32 GetDWord( int32 Index=0 ) const
	{
//		check((Index >= 0) && (Index < 4));
		return DWords[Index];
	}
};



//Address:Port
class IPEndpoint
{
public:
	IPAddress Address;
	uint16 Port;

public:
	static const IPEndpoint SSDPMulticast_v4;

public:
	IPEndpoint()
		: Port(0)
	{}

	IPEndpoint( const IPAddress& InAddress, uint16 InPort )
		: Address(InAddress)
		, Port(InPort)
	{}

	IPEndpoint( IPAddress&& InAddress, uint16 InPort)
		: Address(InAddress)
		, Port(InPort)
	{}

	bool operator==( const IPEndpoint& Other ) const
	{
		return ((Address == Other.Address) && (Port == Other.Port));
	}

	bool operator!=( const IPEndpoint& Other ) const
	{
		return ((Address != Other.Address) || (Port != Other.Port));
	}

	// Exports to text using the internal string buffer (does not require deallocation)
	CACUS_API const char* operator*() const;

#ifdef CORE_API
	// Unreal Tournament
	friend FORCEINLINE FArchive& operator<<( FArchive& Ar, IPEndpoint& Endpoint )
	{
		return Ar << Endpoint.Address << Endpoint.Port;
	}
#endif

#ifdef AF_INET
	inline IPEndpoint( sockaddr_in addr )
		: Address( addr.sin_addr)
	{
		Port = ntohs(addr.sin_port);
	}

	inline sockaddr_in ToSockAddr() const
	{
		sockaddr_in Result = Address.ToSockAddr();
		Result.sin_port = htons(Port);
		return Result;
	}
#endif
#ifdef AF_INET6
	inline IPEndpoint( sockaddr_in6 addr )
		: Address( addr.sin6_addr)
	{
		Port = ntohs(addr.sin6_port);
	}

	inline sockaddr_in6 ToSockAddr6() const
	{
		sockaddr_in6 Result = Address.ToSockAddr6();
		Result.sin6_port = htons(Port);
		return Result;
	}
#endif

};






#endif