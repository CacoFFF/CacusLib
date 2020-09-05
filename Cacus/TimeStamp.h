#pragma once

#include "AppTime.h"

enum ENow { E_Now };
struct CACUS_API FTimestampDateTime
{
	uint64 Year:14;
	uint64 Month:4;
	uint64 Day:5; 
	uint64 Hour:5;
	uint64 Minute:6;
	uint64 Second:6;

	FTimestampDateTime();
	FTimestampDateTime( ENow);

	TCharBuffer<32,char> operator*();
	operator bool()                                       { return *(uint64*)this != 0; }

	int64 Seconds(); //Seconds since 01/01/00 00:00:00
	uint32 Days(); //Days since 01/01/00

	static FTimestampDateTime Parse( const char* Str);
	static bool LeapYear( uint32 Year);
	static uint32 DaysPerMonth( uint32 Month, uint32 Year);
};

#if USES_CACUS_FIELD
class CACUS_API PropertyTimestampDateTime : public CProperty
{
	DECLARE_FIELD(PropertyTimestampDateTime,CProperty)
	SET_PRIMITIVE(FTimestampDateTime)

	PropertyTimestampDateTime( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: CProperty( InName, InParent, InArrayDim, sizeof(FTimestampDateTime), InOffset, InPropertyFlags) {}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	const char* String( void* Object) const;
};

template<> inline CProperty* CreateProperty<FTimestampDateTime>( const char* Name, CStruct* Parent, FTimestampDateTime& Prop, uint32 PropertyFlags)
{
	return CP_CREATE(PropertyTimestampDateTime);
}
#endif



class CActionRecord
{
public:
	double LastActionTime;

	CActionRecord()               : LastActionTime(0)	{}

	void UpdateRecord( double CurTime=FPlatformTime::Seconds())
	{
		LastActionTime = CurTime;
	}

	void ResetRecord()
	{
		LastActionTime = 0;
	}

	//Current timeframe within MaxTime
	bool IsRecordUpdated( double MaxTime, double CurTime=FPlatformTime::Seconds()) 
	{
		double Delta = CurTime - LastActionTime;
		return (Delta >= 0) && (Delta <= MaxTime);
	}

	//Reinterpret 64 bit primitives as a CActionRecord
	static CActionRecord& Reinterpret( int64& Other)   { return *(CActionRecord*)(&Other); }
	static CActionRecord& Reinterpret( uint64& Other)  { return *(CActionRecord*)(&Other); }
	static CActionRecord& Reinterpret( double& Other)  { return *(CActionRecord*)(&Other); }
	//Reinterpret as a 64 bit primitive
	operator int64&()                                  { return *(int64*)(this); }
	operator uint64&()                                 { return *(uint64*)(this); }
	operator double&()                                 { return *(double*)(this); }
};
