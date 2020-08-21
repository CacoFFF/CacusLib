/*=============================================================================
	General.h
	Author: Fernando Velázquez

	General header.
	This header is public domain.
=============================================================================*/

#if defined _M_IX86 || defined _M_X64

inline int32 CFloor( float Value)
{
	int R;
	_MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
	R = _mm_cvtss_si32(_mm_load_ss(&Value));
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
	return R;
}
inline int32 CFloor( double Value)
{
	int R;
	_MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
	R = _mm_cvtsd_si32(_mm_load_sd(&Value));
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
	return R;
}

#else

inline int32 CFloor( float Value)
{
	return (int32)floor(Value);
}

inline int32 CFloor(double Value)
{
	return (int32)floor(Value);
}

#endif