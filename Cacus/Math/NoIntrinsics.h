/*=============================================================================
	NoIntrinsics.h
	Author: Fernando Velázquez

	Wrappers functions when intrinsics is disabled.
=============================================================================*/

typedef union __m128 {
	float     m128_f32[4];
	int8      m128_i8[16];
	int16     m128_i16[8];
	int32     m128_i32[4];
	int64     m128_i64[2];
	uint8     m128_u8[16];
	uint16    m128_u16[8];
	uint32    m128_u32[4];
	uint64    m128_u64[2];
} __m128;

typedef union __m128i {
	float     m128_f32[4];
	int8      m128_i8[16];
	int16     m128_i16[8];
	int32     m128_i32[4];
	int64     m128_i64[2];
	uint8     m128_u8[16];
	uint16    m128_u16[8];
	uint32    m128_u32[4];
	uint64    m128_u64[2];
} __m128i;

inline __m128 _mm_castsi128_ps( __m128i v)
{
	return *(__m128*)&v;
}

inline __m128i _mm_castps_si128( __m128 v)
{
	return *(__m128i*)&v;
}

inline __m128 _mm_setzero_ps()
{
	__m128 v;
	v.m128_f32[0] = 0;
	v.m128_f32[1] = 0;
	v.m128_f32[2] = 0;
	v.m128_f32[3] = 0;
	return v;
}

inline __m128i _mm_setzero_si128()
{
	__m128i v;
	v.m128_i32[0] = 0;
	v.m128_i32[1] = 0;
	v.m128_i32[2] = 0;
	v.m128_i32[3] = 0;
	return v;
}


inline __m128 _mm_loadu_ps( const float* data)
{
	__m128 v;
	v.m128_f32[0] = data[0];
	v.m128_f32[1] = data[1];
	v.m128_f32[2] = data[2];
	v.m128_f32[3] = data[3];
	return v;
}

inline __m128i _mm_loadu_si128( const __m128i* data)
{
	__m128i v;
	v.m128_i32[0] = data->m128_i32[0];
	v.m128_i32[1] = data->m128_i32[1];
	v.m128_i32[2] = data->m128_i32[2];
	v.m128_i32[3] = data->m128_i32[3];
	return v;
}

inline void _mm_storeu_ps( float* store, __m128 data)
{
	store[0] = data.m128_f32[0];
	store[1] = data.m128_f32[1];
	store[2] = data.m128_f32[2];
	store[3] = data.m128_f32[3];
}

inline void _mm_storeu_si128( __m128i* store, __m128i data)
{
	store->m128_i32[0] = data.m128_i32[0];
	store->m128_i32[1] = data.m128_i32[1];
	store->m128_i32[2] = data.m128_i32[2];
	store->m128_i32[3] = data.m128_i32[3];
}

inline __m128i _mm_cvttps_epi32( __m128 data)
{
	__m128i v;
	v.m128_i32[0] = (int)data.m128_f32[0];
	v.m128_i32[1] = (int)data.m128_f32[1];
	v.m128_i32[2] = (int)data.m128_f32[2];
	v.m128_i32[3] = (int)data.m128_f32[3];
	return v;
}

inline __m128i _mm_add_epi32( __m128i A, __m128i B)
{
	__m128i v;
	v.m128_i32[0] = A.m128_i32[0] + B.m128_i32[0];
	v.m128_i32[1] = A.m128_i32[1] + B.m128_i32[1];
	v.m128_i32[2] = A.m128_i32[2] + B.m128_i32[2];
	v.m128_i32[3] = A.m128_i32[3] + B.m128_i32[3];
	return v;
}

inline __m128i _mm_sub_epi32( __m128i A, __m128i B)
{
	__m128i v;
	v.m128_i32[0] = A.m128_i32[0] - B.m128_i32[0];
	v.m128_i32[1] = A.m128_i32[1] - B.m128_i32[1];
	v.m128_i32[2] = A.m128_i32[2] - B.m128_i32[2];
	v.m128_i32[3] = A.m128_i32[3] - B.m128_i32[3];
	return v;
}

inline __m128 _mm_add_ps( __m128 A, __m128 B)
{
	__m128 v;
	v.m128_f32[0] = A.m128_f32[0] + B.m128_f32[0];
	v.m128_f32[1] = A.m128_f32[1] + B.m128_f32[1];
	v.m128_f32[2] = A.m128_f32[2] + B.m128_f32[2];
	v.m128_f32[3] = A.m128_f32[3] + B.m128_f32[3];
	return v;
}

inline __m128 _mm_sub_ps( __m128 A, __m128 B)
{
	__m128 v;
	v.m128_f32[0] = A.m128_f32[0] - B.m128_f32[0];
	v.m128_f32[1] = A.m128_f32[1] - B.m128_f32[1];
	v.m128_f32[2] = A.m128_f32[2] - B.m128_f32[2];
	v.m128_f32[3] = A.m128_f32[3] - B.m128_f32[3];
	return v;
}

inline __m128 _mm_mul_ps( __m128 A, __m128 B)
{
	__m128 v;
	v.m128_f32[0] = A.m128_f32[0] * B.m128_f32[0];
	v.m128_f32[1] = A.m128_f32[1] * B.m128_f32[1];
	v.m128_f32[2] = A.m128_f32[2] * B.m128_f32[2];
	v.m128_f32[3] = A.m128_f32[3] * B.m128_f32[3];
	return v;
}

inline __m128 _mm_div_ps( __m128 A, __m128 B)
{
	__m128 v;
	v.m128_f32[0] = A.m128_f32[0] / B.m128_f32[0];
	v.m128_f32[1] = A.m128_f32[1] / B.m128_f32[1];
	v.m128_f32[2] = A.m128_f32[2] / B.m128_f32[2];
	v.m128_f32[3] = A.m128_f32[3] / B.m128_f32[3];
	return v;
}

inline __m128 _mm_xor_ps( __m128 A, __m128 B)
{
	__m128 v;
	v.m128_i32[0] = A.m128_i32[0] ^ B.m128_i32[0];
	v.m128_i32[1] = A.m128_i32[1] ^ B.m128_i32[1];
	v.m128_i32[2] = A.m128_i32[2] ^ B.m128_i32[2];
	v.m128_i32[3] = A.m128_i32[3] ^ B.m128_i32[3];
	return v;
}