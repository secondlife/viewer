/** 
 * @file llvector4a.cpp
 * @brief SIMD vector implementation
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llmemory.h"
#include "llmath.h"
#include "llquantize.h"

extern const LLQuad F_ZERO_4A		= { 0, 0, 0, 0 };
extern const LLQuad F_APPROXIMATELY_ZERO_4A = { 
	F_APPROXIMATELY_ZERO,
	F_APPROXIMATELY_ZERO,
	F_APPROXIMATELY_ZERO,
	F_APPROXIMATELY_ZERO
};

extern const LLVector4a LL_V4A_ZERO = reinterpret_cast<const LLVector4a&> ( F_ZERO_4A );
extern const LLVector4a LL_V4A_EPSILON = reinterpret_cast<const LLVector4a&> ( F_APPROXIMATELY_ZERO_4A );

/*static */void LLVector4a::memcpyNonAliased16(F32* __restrict dst, const F32* __restrict src, size_t bytes)
{
	assert(src != NULL);
	assert(dst != NULL);
	assert(bytes > 0);
	assert((bytes % sizeof(F32))== 0); 
	ll_assert_aligned(src,16);
	ll_assert_aligned(dst,16);
	assert(bytes%16==0);

	F32* end = dst + (bytes / sizeof(F32) );

	if (bytes > 64)
	{
		F32* begin_64 = LL_NEXT_ALIGNED_ADDRESS_64(dst);
		
		//at least 64 (16*4) bytes before the end of the destination, switch to 16 byte copies
		F32* end_64 = end-16;
		
		_mm_prefetch((char*)begin_64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 128, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 192, _MM_HINT_NTA);
		
		while (dst < begin_64)
		{
			copy4a(dst, src);
			dst += 4;
			src += 4;
		}
		
		while (dst < end_64)
		{
			_mm_prefetch((char*)src + 512, _MM_HINT_NTA);
			_mm_prefetch((char*)dst + 512, _MM_HINT_NTA);
			copy4a(dst, src);
			copy4a(dst+4, src+4);
			copy4a(dst+8, src+8);
			copy4a(dst+12, src+12);
			
			dst += 16;
			src += 16;
		}
	}

	while (dst < end)
	{
		copy4a(dst, src);
		dst += 4;
		src += 4;
	}
}

void LLVector4a::setRotated( const LLRotation& rot, const LLVector4a& vec )
{
	const LLVector4a col0 = rot.getColumn(0);
	const LLVector4a col1 = rot.getColumn(1);
	const LLVector4a col2 = rot.getColumn(2);

	LLVector4a result = _mm_load_ss( vec.getF32ptr() );
	result.splat<0>( result );
	result.mul( col0 );

	{
		LLVector4a yyyy = _mm_load_ss( vec.getF32ptr() +  1 );
		yyyy.splat<0>( yyyy );
		yyyy.mul( col1 ); 
		result.add( yyyy );
	}

	{
		LLVector4a zzzz = _mm_load_ss( vec.getF32ptr() +  2 );
		zzzz.splat<0>( zzzz );
		zzzz.mul( col2 );
		result.add( zzzz );
	}

	*this = result;
}

void LLVector4a::setRotated( const LLQuaternion2& quat, const LLVector4a& vec )
{
	const LLVector4a& quatVec = quat.getVector4a();
	LLVector4a temp; temp.setCross3(quatVec, vec);
	temp.add( temp );
	
	const LLVector4a realPart( quatVec.getScalarAt<3>() );
	LLVector4a tempTimesReal; tempTimesReal.setMul( temp, realPart );

	mQ = vec;
	add( tempTimesReal );
	
	LLVector4a imagCrossTemp; imagCrossTemp.setCross3( quatVec, temp );
	add(imagCrossTemp);
}

void LLVector4a::quantize8( const LLVector4a& low, const LLVector4a& high )
{
	LLVector4a val(mQ);
	LLVector4a delta; delta.setSub( high, low );

	{
		val.clamp(low, high);
		val.sub(low);

		// 8-bit quantization means we can do with just 12 bits of reciprocal accuracy
		const LLVector4a oneOverDelta = _mm_rcp_ps(delta.mQ);
// 		{
// 			static LL_ALIGN_16( const F32 F_TWO_4A[4] ) = { 2.f, 2.f, 2.f, 2.f };
// 			LLVector4a two; two.load4a( F_TWO_4A );
// 
// 			// Here we use _mm_rcp_ps plus one round of newton-raphson
// 			// We wish to find 'x' such that x = 1/delta
// 			// As a first approximation, we take x0 = _mm_rcp_ps(delta)
// 			// Then x1 = 2 * x0 - a * x0^2 or x1 = x0 * ( 2 - a * x0 )
// 			// See Intel AP-803 http://ompf.org/!/Intel_application_note_AP-803.pdf
// 			const LLVector4a recipApprox = _mm_rcp_ps(delta.mQ);
// 			oneOverDelta.setMul( delta, recipApprox );
// 			oneOverDelta.setSub( two, oneOverDelta );
// 			oneOverDelta.mul( recipApprox );
// 		}

		val.mul(oneOverDelta);
		val.mul(*reinterpret_cast<const LLVector4a*>(F_U8MAX_4A));
	}

	val = _mm_cvtepi32_ps(_mm_cvtps_epi32( val.mQ ));

	{
		val.mul(*reinterpret_cast<const LLVector4a*>(F_OOU8MAX_4A));
		val.mul(delta);
		val.add(low);
	}

	{
		LLVector4a maxError; maxError.setMul(delta, *reinterpret_cast<const LLVector4a*>(F_OOU8MAX_4A));
		LLVector4a absVal; absVal.setAbs( val );
		setSelectWithMask( absVal.lessThan( maxError ), F_ZERO_4A, val );
	}	
}

void LLVector4a::quantize16( const LLVector4a& low, const LLVector4a& high )
{
	LLVector4a val(mQ);
	LLVector4a delta; delta.setSub( high, low );

	{
		val.clamp(low, high);
		val.sub(low);

		// 16-bit quantization means we need a round of Newton-Raphson
		LLVector4a oneOverDelta;
		{
			static LL_ALIGN_16( const F32 F_TWO_4A[4] ) = { 2.f, 2.f, 2.f, 2.f };
			ll_assert_aligned(F_TWO_4A,16);
			
			LLVector4a two; two.load4a( F_TWO_4A );

			// Here we use _mm_rcp_ps plus one round of newton-raphson
			// We wish to find 'x' such that x = 1/delta
			// As a first approximation, we take x0 = _mm_rcp_ps(delta)
			// Then x1 = 2 * x0 - a * x0^2 or x1 = x0 * ( 2 - a * x0 )
			// See Intel AP-803 http://ompf.org/!/Intel_application_note_AP-803.pdf
			const LLVector4a recipApprox = _mm_rcp_ps(delta.mQ);
			oneOverDelta.setMul( delta, recipApprox );
			oneOverDelta.setSub( two, oneOverDelta );
			oneOverDelta.mul( recipApprox );
		}

		val.mul(oneOverDelta);
		val.mul(*reinterpret_cast<const LLVector4a*>(F_U16MAX_4A));
	}

	val = _mm_cvtepi32_ps(_mm_cvtps_epi32( val.mQ ));

	{
		val.mul(*reinterpret_cast<const LLVector4a*>(F_OOU16MAX_4A));
		val.mul(delta);
		val.add(low);
	}

	{
		LLVector4a maxError; maxError.setMul(delta, *reinterpret_cast<const LLVector4a*>(F_OOU16MAX_4A));
		LLVector4a absVal; absVal.setAbs( val );
		setSelectWithMask( absVal.lessThan( maxError ), F_ZERO_4A, val );
	}	
}
