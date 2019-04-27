/*=============================================================================
	Math.h
	Author: Fernando Velázquez

	General vector math header.
=============================================================================*/

#ifndef USES_CACUS_MATH
#define USES_CACUS_MATH

#include "../CacusPlatform.h"

//Not precise
#if 1 && ((_MSC_VER >= 1600) || (__GNUC__ >= 4))
	#define CACUS_USES_INTRINSICS 1
	#include "emmintrin.h"
#else
	#include "NoIntrinsics.h"
#endif

#define _mm_pshufd_ps(v,i) _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(v), i))

enum E3D         { E_3D = 0 };
enum EZero       { E_Zero = 0};

struct CFVector4;
struct CIVector4;

struct CFVector4
{
	float X, Y, Z, W;

	CFVector4() {}
	CFVector4( float F);
	CFVector4( float InX, float InY, float InZ, float InW);
	CFVector4( float* data);
	CFVector4( EZero);
	CFVector4( __m128 reg);
	
	CFVector4& operator=( const CFVector4& Other);
	CFVector4 operator+( const CFVector4& Other) const;
	CFVector4 operator-( const CFVector4& Other) const;
	CFVector4 operator*( const CFVector4& Other) const;
	CFVector4 operator/( const CFVector4& Other) const;
	float operator|( const CFVector4& Other) const; //Dot product
	CFVector4 operator*( float F) const;
	CFVector4 operator/( float F) const;

	CFVector4 operator+=( const CFVector4& Other);
	CFVector4 operator-=( const CFVector4& Other);
	CFVector4 operator*=( const CFVector4& Other);
	CFVector4 operator/=( const CFVector4& Other);

	CFVector4 operator-() const;

	bool operator==( const CFVector4& Other) const;
	bool operator<( const CFVector4& Other) const;
	bool operator>( const CFVector4& Other) const;
	bool operator<=( const CFVector4& Other) const;
	bool operator>=( const CFVector4& Other) const;

	operator __m128() const;
	operator __m128i() const;

	float Magnitude() const;
	CFVector4 Abs() const;
	CFVector4 Reciprocal() const;
	CFVector4 Reciprocal_Approx() const;

	CIVector4 Truncate() const;


	static CIVector4 MASK_ABSOLUTE;
	static CIVector4 MASK_SIGNBITS;
};

struct CIVector4
{
	int32 i, j, k, l;

	CIVector4() {}
	CIVector4( int32 In_i, int32 In_j, int32 In_k, int32 In_l);
	CIVector4( int32* data);
	CIVector4( EZero);
	CIVector4( __m128i reg);

	CIVector4 operator=( const CIVector4& Other);
	CIVector4 operator+( const CIVector4& Other) const;
	CIVector4 operator-( const CIVector4& Other) const;
//	CIVector4 operator*( const CIVector4& Other) const; //REQUIRES SSE4
//	CIVector4 operator/( const CIVector4& Other) const;

	CIVector4 operator+=( const CIVector4& Other);
	CIVector4 operator-=( const CIVector4& Other);
//	CIVector4 operator*=( const CIVector4& Other);
//	CIVector4 operator/=( const CIVector4& Other);

	operator __m128() const;
	operator __m128i() const;

	static CIVector4 MASK_3D;
};


#include "CFVector4.h"
#include "CIVector4.h"

#endif
