/** 
 * @file llbboxlocal.h
 * @brief General purpose bounding box class.
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

#ifndef LL_BBOXLOCAL_H
#define LL_BBOXLOCAL_H

#include "v3math.h"

class LLMatrix4;

class LLBBoxLocal
{
public:
    LLBBoxLocal() {}
    LLBBoxLocal( const LLVector3& min, const LLVector3& max ) : mMin( min ), mMax( max ) {}
    // Default copy constructor is OK.

    const LLVector3&    getMin() const                  { return mMin; }
    void                setMin( const LLVector3& min )  { mMin = min; }

    const LLVector3&    getMax() const                  { return mMax; }
    void                setMax( const LLVector3& max )  { mMax = max; }

    LLVector3           getCenter() const               { return (mMax - mMin) * 0.5f + mMin; }
    LLVector3           getExtent() const               { return mMax - mMin; }

    void                addPoint(const LLVector3& p);
    void                addBBox(const LLBBoxLocal& b) { addPoint( b.mMin ); addPoint( b.mMax ); }

    void                expand( F32 delta );

    friend LLBBoxLocal operator*(const LLBBoxLocal& a, const LLMatrix4& b);

private:
    LLVector3 mMin;
    LLVector3 mMax;
};

LLBBoxLocal operator*(const LLBBoxLocal &a, const LLMatrix4 &b);


#endif  // LL_BBOXLOCAL_H
