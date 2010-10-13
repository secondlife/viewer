/** 
 * @file llvector4logical.h
 * @brief LLVector4Logical class header file - Companion class to LLVector4a for logical and bit-twiddling operations
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

#ifndef	LL_VECTOR4LOGICAL_H
#define	LL_VECTOR4LOGICAL_H


////////////////////////////
// LLVector4Logical
////////////////////////////
// This class is incomplete. If you need additional functionality,
// for example setting/unsetting particular elements or performing
// other boolean operations, feel free to implement. If you need
// assistance in determining the most optimal implementation,
// contact someone with SSE experience (Falcon, Richard, Davep, e.g.)
////////////////////////////

static LL_ALIGN_16(const U32 S_V4LOGICAL_MASK_TABLE[4*4]) =
{
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF
};

class LLVector4Logical
{
public:
	
	enum {
		MASK_X = 1,
		MASK_Y = 1 << 1,
		MASK_Z = 1 << 2,
		MASK_W = 1 << 3,
		MASK_XYZ = MASK_X | MASK_Y | MASK_Z,
		MASK_XYZW = MASK_XYZ | MASK_W
	};
	
	// Empty default ctor
	LLVector4Logical() {}
	
	LLVector4Logical( const LLQuad& quad )
	{
		mQ = quad;
	}
	
	// Create and return a mask consisting of the lowest order bit of each element
	inline U32 getGatheredBits() const
	{
		return _mm_movemask_ps(mQ);
	};	
	
	// Invert this mask
	inline LLVector4Logical& invert()
	{
		static const LL_ALIGN_16(U32 allOnes[4]) = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
		mQ = _mm_andnot_ps( mQ, *(LLQuad*)(allOnes) );
		return *this;
	}
	
	inline LLBool32 areAllSet( U32 mask ) const
	{
		return ( getGatheredBits() & mask) == mask;
	}
	
	inline LLBool32 areAllSet() const
	{
		return areAllSet( MASK_XYZW );
	}
		
	inline LLBool32 areAnySet( U32 mask ) const
	{
		return getGatheredBits() & mask;
	}
	
	inline LLBool32 areAnySet() const
	{
		return areAnySet( MASK_XYZW );
	}
	
	inline operator LLQuad() const
	{
		return mQ;
	}

	inline void clear() 
	{
		mQ = _mm_setzero_ps();
	}

	template<int N> void setElement()
	{
		mQ = _mm_or_ps( mQ, *reinterpret_cast<const LLQuad*>(S_V4LOGICAL_MASK_TABLE + 4*N) );
	}
	
private:
	
	LLQuad mQ;
};

#endif //LL_VECTOR4ALOGICAL_H
