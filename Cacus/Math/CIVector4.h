#include "Math.h"
/*=============================================================================
	CIVector4.h
	Author: Fernando Velázquez

	Main CIVector4 implementation.
=============================================================================*/


inline CIVector4::CIVector4( int32 In_i, int32 In_j, int32 In_k, int32 In_l)
	: i(In_i), j(In_j), k(In_k), l(In_l)
{}

inline CIVector4::CIVector4( const int32* data)
{
	_mm_storeu_si128( (__m128i*)&i, _mm_loadu_si128((__m128i*)data));
}

inline CIVector4::CIVector4(EZero)
{
	_mm_storeu_si128( (__m128i*)&i, _mm_setzero_si128());
}

inline CIVector4::CIVector4(__m128i reg)
{
	_mm_storeu_si128( (__m128i*)&i, reg);
}

inline CIVector4 CIVector4::operator=(const CIVector4 & Other)
{
	_mm_storeu_si128( (__m128i*)&i, Other); //Auto-load
	return *this;
}

inline CIVector4 CIVector4::operator+( const CIVector4& Other) const
{
	return CIVector4( _mm_add_epi32( *this, Other));
}

inline CIVector4 CIVector4::operator-( const CIVector4& Other) const
{
	return CIVector4( _mm_sub_epi32( *this, Other));
}
/*
inline CIVector4 CIVector4::operator*( const CIVector4& Other) const
{
	return CIVector4( _mm_mul_epi32( *this, Other));
}

inline CIVector4 CIVector4::operator/( const CIVector4& Other) const
{
	return CIVector4( _mm_div_epi32( *this, Other));
}
*/
inline CIVector4 CIVector4::operator+=( const CIVector4& Other)
{
	return (*this = *this + Other);
}

inline CIVector4 CIVector4::operator-=( const CIVector4& Other)
{
	return (*this = *this - Other);
}
/*
inline CIVector4 CIVector4::operator*=( const CIVector4& Other)
{
	return (*this = *this * Other);
}

inline CIVector4 CIVector4::operator/=( const CIVector4& Other)
{
	return (*this = *this / Other);
}
*/
inline CIVector4::operator __m128() const
{
	return 	_mm_castsi128_ps( _mm_loadu_si128( (__m128i*)&i));
}

inline CIVector4::operator __m128i() const
{
	return _mm_loadu_si128( (__m128i*)&i);
}
