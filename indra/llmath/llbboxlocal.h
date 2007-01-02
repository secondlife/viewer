/** 
 * @file llbboxlocal.h
 * @brief General purpose bounding box class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
