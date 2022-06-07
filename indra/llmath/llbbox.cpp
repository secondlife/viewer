/** 
 * @file llbbox.cpp
 * @brief General purpose bounding box class (Not axis aligned)
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

// self include
#include "llbbox.h"

// library includes
#include "m4math.h"

void LLBBox::addPointLocal(const LLVector3& p)
{
	if (mEmpty)
	{
		mMinLocal = p;
		mMaxLocal = p;
		mEmpty = FALSE;
	}
	else
	{
		mMinLocal.mV[VX] = llmin( p.mV[VX], mMinLocal.mV[VX] );
		mMinLocal.mV[VY] = llmin( p.mV[VY], mMinLocal.mV[VY] );
		mMinLocal.mV[VZ] = llmin( p.mV[VZ], mMinLocal.mV[VZ] );
		mMaxLocal.mV[VX] = llmax( p.mV[VX], mMaxLocal.mV[VX] );
		mMaxLocal.mV[VY] = llmax( p.mV[VY], mMaxLocal.mV[VY] );
		mMaxLocal.mV[VZ] = llmax( p.mV[VZ], mMaxLocal.mV[VZ] );
	}
}

void LLBBox::addPointAgent( LLVector3 p)
{
	p -= mPosAgent;
	p.rotVec( ~mRotation );
	addPointLocal( p );
}


void LLBBox::addBBoxAgent(const LLBBox& b)
{
	if (mEmpty)
	{
		mPosAgent = b.mPosAgent;
		mRotation = b.mRotation;
		mMinLocal.clearVec();
		mMaxLocal.clearVec();
	}
	LLVector3 vertex[8];
	vertex[0].setVec( b.mMinLocal.mV[VX], b.mMinLocal.mV[VY], b.mMinLocal.mV[VZ] );
	vertex[1].setVec( b.mMinLocal.mV[VX], b.mMinLocal.mV[VY], b.mMaxLocal.mV[VZ] );
	vertex[2].setVec( b.mMinLocal.mV[VX], b.mMaxLocal.mV[VY], b.mMinLocal.mV[VZ] );
	vertex[3].setVec( b.mMinLocal.mV[VX], b.mMaxLocal.mV[VY], b.mMaxLocal.mV[VZ] );
	vertex[4].setVec( b.mMaxLocal.mV[VX], b.mMinLocal.mV[VY], b.mMinLocal.mV[VZ] );
	vertex[5].setVec( b.mMaxLocal.mV[VX], b.mMinLocal.mV[VY], b.mMaxLocal.mV[VZ] );
	vertex[6].setVec( b.mMaxLocal.mV[VX], b.mMaxLocal.mV[VY], b.mMinLocal.mV[VZ] );
	vertex[7].setVec( b.mMaxLocal.mV[VX], b.mMaxLocal.mV[VY], b.mMaxLocal.mV[VZ] );

	LLMatrix4 m( b.mRotation );
	m.translate( b.mPosAgent );
	m.translate( -mPosAgent );
	m.rotate( ~mRotation );

	for( S32 i=0; i<8; i++ )
	{
		addPointLocal( vertex[i] * m );
	}
}

LLBBox LLBBox::getAxisAligned() const
{
	// no rotation = axis aligned rotation
	LLBBox aligned(mPosAgent, LLQuaternion(), LLVector3(), LLVector3());

	// add the center point so that it's not empty
	aligned.addPointAgent(mPosAgent);

	// add our BBox
	aligned.addBBoxAgent(*this);

	return aligned;
}

void LLBBox::expand( F32 delta )
{
	mMinLocal.mV[VX] -= delta;
	mMinLocal.mV[VY] -= delta;
	mMinLocal.mV[VZ] -= delta;
	mMaxLocal.mV[VX] += delta;
	mMaxLocal.mV[VY] += delta;
	mMaxLocal.mV[VZ] += delta;
}

LLVector3 LLBBox::localToAgent(const LLVector3& v) const
{
	LLMatrix4 m( mRotation );
	m.translate( mPosAgent );
	return v * m;
}

LLVector3 LLBBox::agentToLocal(const LLVector3& v) const
{
	LLMatrix4 m;
	m.translate( -mPosAgent );
	m.rotate( ~mRotation );  // inverse rotation
	return v * m;
}

LLVector3 LLBBox::localToAgentBasis(const LLVector3& v) const
{
	LLMatrix4 m( mRotation );
	return v * m;
}

LLVector3 LLBBox::agentToLocalBasis(const LLVector3& v) const
{
	LLMatrix4 m( ~mRotation );  // inverse rotation
	return v * m;
}

BOOL LLBBox::containsPointLocal(const LLVector3& p) const
{
	if (  (p.mV[VX] < mMinLocal.mV[VX])
		||(p.mV[VX] > mMaxLocal.mV[VX])
		||(p.mV[VY] < mMinLocal.mV[VY])
		||(p.mV[VY] > mMaxLocal.mV[VY])
		||(p.mV[VZ] < mMinLocal.mV[VZ])
		||(p.mV[VZ] > mMaxLocal.mV[VZ]))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL LLBBox::containsPointAgent(const LLVector3& p) const
{
	LLVector3 point_local = agentToLocal(p);
	return containsPointLocal(point_local);
}

LLVector3 LLBBox::getMinAgent() const
{
	return localToAgent(mMinLocal);
}

LLVector3 LLBBox::getMaxAgent() const
{
	return localToAgent(mMaxLocal);
}

/*
LLBBox operator*(const LLBBox &a, const LLMatrix4 &b)
{
	return LLBBox( a.mMin * b, a.mMax * b );
}
*/

void LLBBoxHelper::getBoundTriangle(unsigned char flag, LLVector3 &t0, LLVector3 &t1, LLVector3 &t2, std::function<void(LLVector3 &pos)> opfn)
{
    static const unsigned char one_bits[] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
    unsigned char nbits = one_bits[flag & 0x0f] + one_bits[flag >> 4];
    llassert_always_msg(3 == nbits, "The number of requested point should be equel 3");

    static const LLVector3 vertices[] = {
        {1,1,1}, {1,1,-1}, {-1,1,-1}, {-1,1,1}, {1,-1,1}, {1,-1,-1}, {-1,-1,-1}, {-1,-1,1}
    };

    LLVector3 * t_arr[] = { &t0, &t1, &t2 };

    for (unsigned char c = 0, n = 1, k = 0; k < 3; n <<= 1, ++c)
    {
        if (flag & n) {
            *t_arr[k] = vertices[c];
            opfn(*t_arr[k]);
            k++;
        }
    }
}
