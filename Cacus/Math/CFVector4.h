#include "Math.h"
/*=============================================================================
	CFVector4.h
	Author: Fernando Velázquez

	Main CFVector4 implementation.
=============================================================================*/


inline CFVector4::CFVector4( float F)
{
	_mm_storeu_ps( &X, _mm_load1_ps( &F));
}
inline CFVector4::CFVector4( float InX, float InY, float InZ, float InW)
	: X(InX), Y(InY), Z(InZ), W(InW)
{}
inline CFVector4::CFVector4( float* data)
{
	_mm_storeu_ps( &X, _mm_loadu_ps(data)); 
}
inline CFVector4::CFVector4( EZero)
{
	_mm_storeu_ps( &X, _mm_setzero_ps());
}
inline CFVector4::CFVector4( __m128 reg)
{
	_mm_storeu_ps( &X, reg); 
}


inline CFVector4& CFVector4::operator=( const CFVector4& Other)
{
	_mm_storeu_ps( &X, Other); //Auto-load
	return *this;
}

inline CFVector4 CFVector4::operator+( const CFVector4& Other) const
{	return CFVector4( _mm_add_ps( *this, Other));	}
inline CFVector4 CFVector4::operator-( const CFVector4& Other) const
{	return CFVector4( _mm_sub_ps( *this, Other));	}
inline CFVector4 CFVector4::operator*( const CFVector4& Other) const
{	return CFVector4( _mm_mul_ps( *this, Other));	}
inline CFVector4 CFVector4::operator/( const CFVector4& Other) const
{	return CFVector4( _mm_div_ps( *this, Other));	}

inline float CFVector4::operator|( const CFVector4& Other) const
{
	float DotProduct;
#ifdef CACUS_USES_INTRINSICS
	__m128 v = _mm_mul_ps(*this,Other);
	__m128 w = _mm_pshufd_ps( v, 0b10110001); //x,y,z,w >> y,x,w,z
	v = _mm_add_ps( v, w); // x+y,-,z+w,-
	w = _mm_pshufd_ps( v, 0b00000010); // >> z+w,-,-,-
	_mm_store_ss( &DotProduct, _mm_add_ss(v,w));
#else
	DotProduct = X*Other.X + Y*Other.Y + Z*Other.Z + W*Other.W;
#endif
	return DotProduct;
}

inline CFVector4 CFVector4::operator*(float F) const
{	return *this * CFVector4(F);	}
inline CFVector4 CFVector4::operator/(float F) const
{	return *this / CFVector4(F);	}



inline CFVector4 CFVector4::operator+=( const CFVector4& Other)
{	return (*this = *this + Other);	}
inline CFVector4 CFVector4::operator-=( const CFVector4& Other)
{	return (*this = *this - Other);	}
inline CFVector4 CFVector4::operator*=( const CFVector4& Other)
{	return (*this = *this * Other);	}
inline CFVector4 CFVector4::operator/=( const CFVector4& Other)
{	return (*this = *this / Other);	}

inline CFVector4 CFVector4::operator-() const
{
	return CFVector4( _mm_xor_ps( *this, MASK_SIGNBITS));
}

#ifdef CACUS_USES_INTRINSICS
inline bool CFVector4::operator==(const CFVector4 & Other) const
{	return _mm_movemask_ps( _mm_cmpeq_ps( *this, Other) ) == 0b1111;	}
inline bool CFVector4::operator<( const CFVector4 & Other) const
{	return _mm_movemask_ps( _mm_cmplt_ps( *this, Other) ) == 0b1111;	}
inline bool CFVector4::operator>( const CFVector4 & Other) const
{	return _mm_movemask_ps( _mm_cmpgt_ps( *this, Other) ) == 0b1111;	}
inline bool CFVector4::operator<=( const CFVector4 & Other) const
{	return _mm_movemask_ps( _mm_cmple_ps( *this, Other) ) == 0b1111;	}
inline bool CFVector4::operator>=( const CFVector4 & Other) const
{	return _mm_movemask_ps( _mm_cmpge_ps( *this, Other) ) == 0b1111;	}
#else
inline bool CFVector4::operator==( const CFVector4 & Other) const
{	return (X == Other.X) && (Y == Other.Y) && (Z == Other.Z) && (W == Other.W);	}
inline bool CFVector4::operator<( const CFVector4 & Other) const
{	return (X < Other.X) && (Y < Other.Y) && (Z < Other.Z) && (W < Other.W);	}
inline bool CFVector4::operator>( const CFVector4 & Other) const
{	return (X > Other.X) && (Y > Other.Y) && (Z > Other.Z) && (W > Other.W);	}
inline bool CFVector4::operator<=( const CFVector4 & Other) const
{	return (X <= Other.X) && (Y <= Other.Y) && (Z <= Other.Z) && (W <= Other.W);	}
inline bool CFVector4::operator>=( const CFVector4 & Other) const
{	return (X >= Other.X) && (Y >= Other.Y) && (Z >= Other.Z) && (W >= Other.W);	}
#endif

inline CFVector4::operator __m128() const
{
	return _mm_loadu_ps( &X);
}

inline CFVector4::operator __m128i() const
{
	return _mm_castps_si128( _mm_loadu_ps( &X));
}

inline CFVector4 CFVector4::Abs() const
{
	return CFVector4( _mm_and_ps( MASK_ABSOLUTE, *this));
}

inline CFVector4 CFVector4::Reciprocal() const
{
	return CFVector4(1.f) / *this;
}

inline CFVector4 CFVector4::Reciprocal_Approx() const
{
	__m128 z = _mm_rcp_ps( *this); //z = 1/x estimate
	__m128 Rcp = _mm_sub_ps( _mm_add_ps( z, z), _mm_mul_ps( *this, _mm_mul_ps( z, z))); //2z-xzz
	return CFVector4(Rcp); //~= 1/x to 0.000012%
}

inline CIVector4 CFVector4::Truncate() const
{
	return CIVector4( _mm_cvttps_epi32( *this ));
}

inline float CFVector4::Magnitude() const
{
	float M;
	float Dot = *this | *this;
	_mm_store_ss( &M, _mm_sqrt_ss( _mm_load_ss(&Dot)));
	return M;
}
