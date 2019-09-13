/*=============================================================================
	Math.h
	Author: Fernando Velázquez

	General vector math header.
=============================================================================*/

#ifndef USES_CACUS_MATH
#define USES_CACUS_MATH

#include "../CacusPlatform.h"

/** Compiler filter
 *
 * While this code will never compile if SSE intrinsics are not supported, it'll
 * give the coder the opportunity to include this header and then find workarounds
 * to whatever functionaliy is needed.
 * (conditions below not precise)
 */
#if ((_MSC_VER >= 1600) || (__GNUC__ >= 4))
	#if (__GNUC__ && __GNUC__ <= 6)
		#warning "GCC needs to be at least 7.x for decent vectorization, expect unoptimized code."
	#endif
	#include "emmintrin.h"
#else
	#define CACUS_NO_INTRINSICS
	class __m128;
	class __m128i;
#endif

#define _mm_pshufd_ps(v,i) _mm_castsi128_ps( _mm_shuffle_epi32( _mm_castps_si128(v), i))

enum E3D         { E_3D = 0 };
enum EZero       { E_Zero = 0};
enum EX          { E_X = 0};

struct CFVector4;
struct CIVector2;
struct CIVector4;

struct CFVector4
{
	float X, Y, Z, W;

	//Constructors
	CFVector4() {}
	CFVector4( float F);
	CFVector4( float InX, EX);
	CFVector4( float InX, float InY, float InZ, float InW);
	CFVector4( const float* data);
	CFVector4( const float* data, E3D);
	CFVector4( EZero);
	CFVector4( __m128 reg);
	
	CFVector4& operator=( const CFVector4& Other);
	CFVector4 operator+( const CFVector4& Other) const;
	CFVector4 operator-( const CFVector4& Other) const;
	CFVector4 operator*( const CFVector4& Other) const;
	CFVector4 operator/( const CFVector4& Other) const;
	float operator|( const CFVector4& Other) const; //Dot product
	CFVector4 operator&( const CFVector4& Other) const;
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
	float SquareMagnitude() const;
	CFVector4 Abs() const;
	CFVector4 Reciprocal() const;
	CFVector4 Reciprocal_Approx() const;
	CFVector4 Normal_Approx() const;

	CIVector4 Truncate_SSE() const;
	CIVector4 Truncate() const;
	int MSB_Mask() const;

	//Global methods
	friend CFVector4 Min( const CFVector4& A, const CFVector4& B);
	friend CFVector4 Max( const CFVector4& A, const CFVector4& B);
	friend CFVector4 Clamp( const CFVector4& V, const CFVector4& Min, const CFVector4& Max);

	//Internals
	float _X() const;
	CFVector4 _CoordsSum() const;
	CFVector4 _Dot( const CFVector4& Other) const;

	//Constants
	static CIVector4 MASK_ABSOLUTE;
	static CIVector4 MASK_SIGNBITS;
};

struct CIVector2
{
	int32 i, j;

	CIVector2() {}
	CIVector2( int32 In_i, int32 In_j);

};

struct CIVector4
{
	int32 i, j, k, l;

	CIVector4() {}
	CIVector4( int32 In_i, int32 In_j, int32 In_k, int32 In_l);
	CIVector4( const int32* data);
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

	int MSB_Mask() const;

	static CIVector4 MASK_3D;
};

#ifndef CACUS_NO_INTRINSICS
	#include "CFVector4.h"
	#include "CIVector2.h"
	#include "CIVector4.h"
#endif


#endif
