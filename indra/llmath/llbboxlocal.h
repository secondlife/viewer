/** 
 * @file llbboxlocal.h
 * @brief General purpose bounding box class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

	const LLVector3&	getMin() const					{ return mMin; }
	void				setMin( const LLVector3& min )	{ mMin = min; }

	const LLVector3&	getMax() const					{ return mMax; }
	void				setMax( const LLVector3& max )	{ mMax = max; }

	LLVector3			getCenter() const				{ return (mMax - mMin) * 0.5f + mMin; }
	LLVector3			getExtent() const				{ return mMax - mMin; }

	BOOL				containsPoint(const LLVector3& p) const;
	BOOL				intersects(const LLBBoxLocal& b) const;

	void				addPoint(const LLVector3& p);
	void				addBBox(const LLBBoxLocal& b) {	addPoint( b.mMin );	addPoint( b.mMax ); }

	void				expand( F32 delta );

	friend LLBBoxLocal operator*(const LLBBoxLocal& a, const LLMatrix4& b);

private:
	LLVector3 mMin;
	LLVector3 mMax;
};

LLBBoxLocal operator*(const LLBBoxLocal &a, const LLMatrix4 &b);


#endif  // LL_BBOXLOCAL_H
