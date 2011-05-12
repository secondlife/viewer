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

#include "llmath.h"

static LL_ALIGN_16(const F32 M_IDENT_3A[12]) = 
												{	1.f, 0.f, 0.f, 0.f, // Column 1
													0.f, 1.f, 0.f, 0.f, // Column 2
													0.f, 0.f, 1.f, 0.f }; // Column 3

extern const LLMatrix3a LL_M3A_IDENTITY = *reinterpret_cast<const LLMatrix3a*> (M_IDENT_3A);

void LLMatrix3a::setMul( const LLMatrix3a& lhs, const LLMatrix3a& rhs )
{
	const LLVector4a col0 = lhs.getColumn(0);
	const LLVector4a col1 = lhs.getColumn(1);
	const LLVector4a col2 = lhs.getColumn(2);

	for ( int i = 0; i < 3; i++ )
	{
		LLVector4a xxxx = _mm_load_ss( rhs.mColumns[i].getF32ptr() );
		xxxx.splat<0>( xxxx );
		xxxx.mul( col0 );

		{
			LLVector4a yyyy = _mm_load_ss( rhs.mColumns[i].getF32ptr() +  1 );
			yyyy.splat<0>( yyyy );
			yyyy.mul( col1 ); 
			xxxx.add( yyyy );
		}

		{
			LLVector4a zzzz = _mm_load_ss( rhs.mColumns[i].getF32ptr() +  2 );
			zzzz.splat<0>( zzzz );
			zzzz.mul( col2 );
			xxxx.add( zzzz );
		}

		xxxx.store4a( mColumns[i].getF32ptr() );
	}
	
}

/*static */void LLMatrix3a::batchTransform( const LLMatrix3a& xform, const LLVector4a* src, int numVectors, LLVector4a* dst )
{
	const LLVector4a col0 = xform.getColumn(0);
	const LLVector4a col1 = xform.getColumn(1);
	const LLVector4a col2 = xform.getColumn(2);
	const LLVector4a* maxAddr = src + numVectors;

	if ( numVectors & 0x1 )
	{
		LLVector4a xxxx = _mm_load_ss( (const F32*)src );
		LLVector4a yyyy = _mm_load_ss( (const F32*)src + 1 );
		LLVector4a zzzz = _mm_load_ss( (const F32*)src + 2 );
		xxxx.splat<0>( xxxx );
		yyyy.splat<0>( yyyy );
		zzzz.splat<0>( zzzz );
		xxxx.mul( col0 );
		yyyy.mul( col1 ); 
		zzzz.mul( col2 );
		xxxx.add( yyyy );
		xxxx.add( zzzz );
		xxxx.store4a( (F32*)dst );
		src++;
		dst++;
	}


	numVectors >>= 1;
	while ( src < maxAddr )
	{
		_mm_prefetch( (const char*)(src + 32 ), _MM_HINT_NTA );
		_mm_prefetch( (const char*)(dst + 32), _MM_HINT_NTA );
		LLVector4a xxxx = _mm_load_ss( (const F32*)src );
		LLVector4a xxxx1= _mm_load_ss( (const F32*)(src + 1) );

		xxxx.splat<0>( xxxx );
		xxxx1.splat<0>( xxxx1 );
		xxxx.mul( col0 );
		xxxx1.mul( col0 );

		{
			LLVector4a yyyy = _mm_load_ss( (const F32*)src + 1 );
			LLVector4a yyyy1 = _mm_load_ss( (const F32*)(src + 1) + 1);
			yyyy.splat<0>( yyyy );
			yyyy1.splat<0>( yyyy1 );
			yyyy.mul( col1 );
			yyyy1.mul( col1 );
			xxxx.add( yyyy );
			xxxx1.add( yyyy1 );
		}

		{
			LLVector4a zzzz = _mm_load_ss( (const F32*)(src) + 2 );
			LLVector4a zzzz1 = _mm_load_ss( (const F32*)(++src) + 2 );
			zzzz.splat<0>( zzzz );
			zzzz1.splat<0>( zzzz1 );
			zzzz.mul( col2 );
			zzzz1.mul( col2 );
			xxxx.add( zzzz );
			xxxx1.add( zzzz1 );
		}

		xxxx.store4a(dst->getF32ptr());
		src++;
		dst++;

		xxxx1.store4a((F32*)dst++);
	}
}
