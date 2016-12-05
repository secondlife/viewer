/** 
 * @file llmatrix4a.h
 * @brief LLMatrix4a class header file - memory aligned and vectorized 4x4 matrix
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef	LL_LLMATRIX4A_H
#define	LL_LLMATRIX4A_H

#include "llvector4a.h"
#include "m4math.h"
#include "m3math.h"

class LLMatrix4a
{
public:
	LL_ALIGN_16(LLVector4a mMatrix[4]);

	inline void clear()
	{
		mMatrix[0].clear();
		mMatrix[1].clear();
		mMatrix[2].clear();
		mMatrix[3].clear();
	}

	inline void loadu(const LLMatrix4& src)
	{
		mMatrix[0] = _mm_loadu_ps(src.mMatrix[0]);
		mMatrix[1] = _mm_loadu_ps(src.mMatrix[1]);
		mMatrix[2] = _mm_loadu_ps(src.mMatrix[2]);
		mMatrix[3] = _mm_loadu_ps(src.mMatrix[3]);
		
	}

	inline void loadu(const LLMatrix3& src)
	{
		mMatrix[0].load3(src.mMatrix[0]);
		mMatrix[1].load3(src.mMatrix[1]);
		mMatrix[2].load3(src.mMatrix[2]);
		mMatrix[3].set(0,0,0,1.f);
	}

	inline void add(const LLMatrix4a& rhs)
	{
		mMatrix[0].add(rhs.mMatrix[0]);
		mMatrix[1].add(rhs.mMatrix[1]);
		mMatrix[2].add(rhs.mMatrix[2]);
		mMatrix[3].add(rhs.mMatrix[3]);
	}

	inline void setRows(const LLVector4a& r0, const LLVector4a& r1, const LLVector4a& r2)
	{
		mMatrix[0] = r0;
		mMatrix[1] = r1;
		mMatrix[2] = r2;
	}

	inline void setMul(const LLMatrix4a& m, const F32 s)
	{
		mMatrix[0].setMul(m.mMatrix[0], s);
		mMatrix[1].setMul(m.mMatrix[1], s);
		mMatrix[2].setMul(m.mMatrix[2], s);
		mMatrix[3].setMul(m.mMatrix[3], s);
	}

	inline void setLerp(const LLMatrix4a& a, const LLMatrix4a& b, F32 w)
	{
		LLVector4a d0,d1,d2,d3;
		d0.setSub(b.mMatrix[0], a.mMatrix[0]);
		d1.setSub(b.mMatrix[1], a.mMatrix[1]);
		d2.setSub(b.mMatrix[2], a.mMatrix[2]);
		d3.setSub(b.mMatrix[3], a.mMatrix[3]);

		// this = a + d*w
		
		d0.mul(w);
		d1.mul(w);
		d2.mul(w);
		d3.mul(w);

		mMatrix[0].setAdd(a.mMatrix[0],d0);
		mMatrix[1].setAdd(a.mMatrix[1],d1);
		mMatrix[2].setAdd(a.mMatrix[2],d2);
		mMatrix[3].setAdd(a.mMatrix[3],d3);
	}

	inline void rotate(const LLVector4a& v, LLVector4a& res)
	{
		LLVector4a y,z;

		res = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
		y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
		z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
		
		res.mul(mMatrix[0]);
		y.mul(mMatrix[1]);
		z.mul(mMatrix[2]);

		res.add(y);
		res.add(z);
	}

	inline void affineTransformSSE(const LLVector4a& v, LLVector4a& res)
	{
		LLVector4a x,y,z;

		x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
		y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
		z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
		
		x.mul(mMatrix[0]);
		y.mul(mMatrix[1]);
		z.mul(mMatrix[2]);

		x.add(y);
		z.add(mMatrix[3]);
		res.setAdd(x,z);
	}

    inline void affineTransformNonSSE(const LLVector4a& v, LLVector4a& res)
    {
        F32 x = v[0] * mMatrix[0][0] + v[1] * mMatrix[1][0] + v[2] * mMatrix[2][0] + mMatrix[3][0];
        F32 y = v[0] * mMatrix[0][1] + v[1] * mMatrix[1][1] + v[2] * mMatrix[2][1] + mMatrix[3][1];
        F32 z = v[0] * mMatrix[0][2] + v[1] * mMatrix[1][2] + v[2] * mMatrix[2][2] + mMatrix[3][2];
        F32 w = 1.0f;
        res.set(x,y,z,w);
    }

	inline void affineTransform(const LLVector4a& v, LLVector4a& res)
    {
        affineTransformSSE(v,res);
    }
};

inline LLVector4a rowMul(const LLVector4a &row, const LLMatrix4a &mat)
{
    LLVector4a result;
    result = _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(0, 0, 0, 0)), mat.mMatrix[0]);
    result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(1, 1, 1, 1)), mat.mMatrix[1]));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(2, 2, 2, 2)), mat.mMatrix[2]));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(row, row, _MM_SHUFFLE(3, 3, 3, 3)), mat.mMatrix[3]));
    return result;
}

inline void matMul(const LLMatrix4a &a, const LLMatrix4a &b, LLMatrix4a &res)
{
    LLVector4a row0 = rowMul(a.mMatrix[0], b);
    LLVector4a row1 = rowMul(a.mMatrix[1], b);
    LLVector4a row2 = rowMul(a.mMatrix[2], b);
    LLVector4a row3 = rowMul(a.mMatrix[3], b);

    res.mMatrix[0] = row0;
    res.mMatrix[1] = row1;
    res.mMatrix[2] = row2;
    res.mMatrix[3] = row3;
}

#endif
