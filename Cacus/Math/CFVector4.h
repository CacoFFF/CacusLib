/*=============================================================================
	CFVector4.h
	Author: Fernando Velázquez

	Main CFVector4 implementation.
	This header is public domain.
=============================================================================*/


inline CFVector4::CFVector4( float F)
{
	_mm_storeu_ps( &X, _mm_load1_ps( &F));
}
inline CFVector4::CFVector4( float InX, EX)
{
	_mm_storeu_ps( &X, _mm_load_ss( &InX));
}
inline CFVector4::CFVector4( float InX, float InY, float InZ, float InW)
	: X(InX), Y(InY), Z(InZ), W(InW)
{}
inline CFVector4::CFVector4( const float* data)
{
	_mm_storeu_ps( &X, _mm_loadu_ps(data)); 
}
inline CFVector4::CFVector4( const float* data, E3D)
{
	_mm_storeu_ps( &X, _mm_and_ps( _mm_loadu_ps(data), CIVector4::MASK_3D) ); 
}
inline CFVector4::CFVector4( const float* data, EXYXY)
{
	__m128 xy = _mm_loadl_pi( _mm_setzero_ps(), (__m64*)data);
	_mm_storeu_ps( &X, _mm_movelh_ps( xy, xy));
}
inline CFVector4::CFVector4( EZero)
{
	_mm_storeu_ps( &X, _mm_setzero_ps());
}
inline CFVector4::CFVector4( CF_reg128 reg)
{
	_mm_storeu_ps( &X, reg); 
}


inline CFVector4& CFVector4::operator=( const CFVector4& Other)
{
	_mm_storeu_ps( &X, Other.reg_f()); //Auto-load
	return *this;
}

inline CFVector4 CFVector4::operator+( const CFVector4& Other) const
{	return _mm_add_ps( reg_f(), Other.reg_f());	}
inline CFVector4 CFVector4::operator-( const CFVector4& Other) const
{	return _mm_sub_ps( reg_f(), Other.reg_f());	}
inline CFVector4 CFVector4::operator*( const CFVector4& Other) const
{	return _mm_mul_ps( reg_f(), Other.reg_f());	}
inline CFVector4 CFVector4::operator/( const CFVector4& Other) const
{	return _mm_div_ps( reg_f(), Other.reg_f());	}

inline float CFVector4::operator|( const CFVector4& Other) const
{	return GetX(_Dot(Other));	}

inline CFVector4 CFVector4::operator&( const CFVector4& Other) const
{	return _mm_and_ps( reg_f(), Other.reg_f());	}

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
	return CFVector4( _mm_xor_ps( this->reg_f(), MASK_SIGNBITS));
}

inline bool CFVector4::operator==(const CFVector4& Other) const
{	return _mm_movemask_ps( _mm_cmpeq_ps( reg_f(), Other.reg_f()) ) == 0b1111;	}
inline bool CFVector4::operator<( const CFVector4& Other) const
{	return _mm_movemask_ps( _mm_cmplt_ps( reg_f(), Other.reg_f()) ) == 0b1111;	}
inline bool CFVector4::operator>( const CFVector4& Other) const
{	return _mm_movemask_ps( _mm_cmpgt_ps( reg_f(), Other.reg_f()) ) == 0b1111;	}
inline bool CFVector4::operator<=( const CFVector4& Other) const
{	return _mm_movemask_ps( _mm_cmple_ps( reg_f(), Other.reg_f()) ) == 0b1111;	}
inline bool CFVector4::operator>=( const CFVector4& Other) const
{	return _mm_movemask_ps( _mm_cmpge_ps( reg_f(), Other.reg_f()) ) == 0b1111;	}


inline CF_reg128 CFVector4::reg_f() const
{
	return _mm_loadu_ps( &X);
}

inline CI_reg128 CFVector4::reg_i() const
{
	return _mm_castps_si128( _mm_loadu_ps( &X));
}

inline CFVector4 CFVector4::Abs() const
{
	return CFVector4( _mm_and_ps( MASK_ABSOLUTE, reg_f() ));
}

inline CFVector4 CFVector4::Reciprocal() const
{
	return CFVector4(1.f) / *this;
}

inline CFVector4 CFVector4::Reciprocal_Fast() const
{
	return _mm_rcp_ps( reg_f() ); //z = 1/x estimate
}

inline CFVector4 CFVector4::Reciprocal_Approx() const
{
	__m128 z = _mm_rcp_ps( reg_f() ); //z = 1/x estimate
	__m128 Rcp = _mm_sub_ps( _mm_add_ps( z, z), _mm_mul_ps( reg_f() , _mm_mul_ps( z, z))); //2z-xzz
	return CFVector4(Rcp); //~= 1/x to 0.000012%
}

inline CFVector4 CFVector4::Normal_Approx() const
{
	return *this * _mm_rsqrt_ss( _Dot( *this ));
}

inline CIVector4 CFVector4::Int() const
{
	return _mm_cvttps_epi32( reg_f() );
}

inline int CFVector4::MSB_Mask() const
{
	return _mm_movemask_ps( reg_f() );
}

inline float CFVector4::Magnitude() const
{
	return GetX( _mm_sqrt_ss( _Dot(*this) ));
}

inline float CFVector4::SquareMagnitude() const
{
	return GetX(_Dot(*this));
}


//*****************************
//Global methods
inline CFVector4 Min( const CFVector4& A, const CFVector4& B)
{	return _mm_min_ps( A.reg_f(), B.reg_f());	}
inline CFVector4 Max( const CFVector4& A, const CFVector4& B)
{	return _mm_max_ps( A.reg_f(), B.reg_f());	}
inline CFVector4 Clamp(const CFVector4& V, const CFVector4& Min, const CFVector4& Max)
{	return ::Min( ::Max( V, Min), Max);	}



//*****************************
//Internals
inline CF_reg128 CFVector4::_CoordsSum() const
{
	__m128 v = reg_f();
	__m128 w = _mm_shuffle_ps( v, v, 0b10110001); //x,y,z,w >> y,x,w,z
//	__m128 w = _mm_pshufd_ps( v, 0b10110001); //SSE2!
	v = _mm_add_ps( v, w); // x+y,-,z+w,-
	w = _mm_movehl_ps( w, v); // >> z+w,-,-,-
	w = _mm_add_ss( w, v); // x+y+z+w
	return w;
}

inline CF_reg128 CFVector4::_Dot( const CFVector4& Other) const
{
	return (*this * Other)._CoordsSum();
}

