/*=============================================================================
	IPv6.cpp

	IPv6/IPv4 address container implementation.

	Author: Fernando Velazquez
=============================================================================*/

// No need to do bounds checking here
#undef check
#define check() 

#include "CacusBase.h"

#include "IPv6.h"
#include "TCharBuffer.h"

bool GIPv6 = 0;

const IPAddress IPAddress::Any(0,0,0,0,0,0,0,0);
const IPAddress IPAddress::InternalLoopback(0,0,0,0,0,0,0,1);
const IPAddress IPAddress::InternalLoopback_v4(127,0,0,1);
const IPAddress IPAddress::LanBroadcast(0xFF02,0,0,0,0,0,0,1);
const IPAddress IPAddress::LanBroadcast_v4(224,0,0,1);

/** RFC 4291 notes:

	2.5.5.2. IPv4-Mapped IPv6 Address 

		|                80 bits               | 16 |      32 bits        |
		+--------------------------------------+--------------------------+
		|0000..............................0000|FFFF|    IPv4 address     |
		+--------------------------------------+----+---------------------+
*/

static int32 CountZeros( const IPAddress& Address, int32& i)
{
	int32 iStart = i;
	for ( ; i<8 && !Address.GetWord(i); i++);
	return i - iStart;
}

const char *IPAddress::operator*() const
{
	if ( IsIPv4() )
		return CSprintf( "%i.%i.%i.%i", (int32)Bytes[3], (int32)Bytes[2], (int32)Bytes[1], (int32)Bytes[0]);

	/** RFC 5952 notes:
	
		4.2.  "::" Usage

		4.2.1.  Shorten as Much as Possible
		   The use of the symbol "::" MUST be used to its maximum capability.
		   For example, 2001:db8:0:0:0:0:2:1 must be shortened to 2001:db8::2:1.
		   Likewise, 2001:db8::0:1 is not acceptable, because the symbol "::"
		   could have been used to produce a shorter representation 2001:db8::1.

		4.2.2.  Handling One 16-Bit 0 Field
		   The symbol "::" MUST NOT be used to shorten just one 16-bit 0 field.
		   For example, the representation 2001:db8:0:1:1:1:1:1 is correct, but
		   2001:db8::1:1:1:1:1 is not correct.

		4.2.3.  Choice in Placement of "::"
		   When there is an alternative choice in the placement of a "::", the
		   longest run of consecutive 16-bit 0 fields MUST be shortened (i.e.,
		   the sequence with three consecutive zero fields is shortened in 2001:
		   0:0:1:0:0:0:1).  When the length of the consecutive 16-bit 0 fields
		   are equal (i.e., 2001:db8:0:0:1:0:0:1), the first sequence of zero
		   bits MUST be shortened.  For example, 2001:db8::1:0:0:1 is correct
		   representation.
	*/
	int32 i = 0;
	int32 LongestCount = 1; // Only consider sequences larger than 1
	int32 LongestStart = 0 - LongestCount;
	while ( i < 8 )
	{
		int32 Start     = i;
		int32 ZeroCount = CountZeros(*this,i);
		if ( ZeroCount > LongestCount )
		{
			LongestCount = ZeroCount;
			LongestStart = Start;
		}
		i++;
	}

	/**  RFC 5952 notes:

		4.1.  Handling Leading Zeros in a 16-Bit Field
		   Leading zeros MUST be suppressed.  For example, 2001:0db8::0001 is
		   not acceptable and must be represented as 2001:db8::1.  A single 16-
		   bit 0000 field MUST be represented as 0.

		4.3.  Lowercase
		   The characters "a", "b", "c", "d", "e", and "f" in an IPv6 address
		   MUST be represented in lowercase.
	*/
	uint8 PrintType[8];
	int32 LongestEnd = LongestStart + LongestCount;
	for ( i=0; i<8; i++)
		PrintType[i] = (i<LongestStart || i>=LongestEnd) ? 1  // Export Word to hex
		             : (i == LongestStart)               ? 2  // Add an empty string
		             :                                     0; // Ignore

	TChar8Buffer<64>::TWriter LocalWriter;
	for ( i=7; i>=0; i--)
		if ( PrintType[i] != 0 )
		{
			if ( (LocalWriter.GetPos() != 0) || (PrintType[i] == 2) )
				LocalWriter << ":";
			if ( PrintType[i] == 1 )
			{
				char HexBuffer[8];
				sprintf( HexBuffer, "%x", (int)Words[i]);
				LocalWriter << HexBuffer;
			}
		}

	if ( LocalWriter.GetBuffer() == ":")
		LocalWriter << ":";

	return CopyToBuffer(*LocalWriter.GetBuffer());
}


const char* IPEndpoint::operator*() const
{
	if ( Port == 0 )
		return *Address;

	if ( Address.IsIPv4() ) //Single allocation
		return CSprintf( "%i.%i.%i.%i:%i", (int)Address.GetByte(3), (int)Address.GetByte(2), (int)Address.GetByte(1), (int)Address.GetByte(0), (int)Port);

	return CSprintf("[%s]:%i", *Address, (int)Port);
}
