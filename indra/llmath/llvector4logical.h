/** 
 * @file llvector4logical.h
 * @brief LLVector4Logical class header file - Companion class to LLVector4a for logical and bit-twiddling operations
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2007-2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
