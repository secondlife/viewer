/** 
 * @file llrect.h
 * @brief A rectangle in GL coordinates, with bottom,left = 0,0
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


#ifndef LL_LLRECT_H
#define LL_LLRECT_H

#include <iostream>
#include "llmath.h"
#include "llsd.h"

// Top > Bottom due to GL coords
template <class Type> class LLRectBase
{
public:
	Type		mLeft;
	Type		mTop;
	Type		mRight;
	Type		mBottom;

	// Note: follows GL_QUAD conventions: the top and right edges are not considered part of the rect
	Type		getWidth()	const { return mRight - mLeft; }
	Type		getHeight()	const { return mTop - mBottom; }
	Type		getCenterX() const { return (mLeft + mRight) / 2; }
	Type		getCenterY() const { return (mTop + mBottom) / 2; }

	LLRectBase():	mLeft(0), mTop(0), mRight(0), mBottom(0)
	{}

	LLRectBase(const LLRectBase &r):
	mLeft(r.mLeft), mTop(r.mTop), mRight(r.mRight), mBottom(r.mBottom)
	{}

	LLRectBase(Type left, Type top, Type right, Type bottom):
	mLeft(left), mTop(top), mRight(right), mBottom(bottom)
	{}

	LLRectBase(const LLSD& sd)
	{
		setValue(sd);
	}

	const LLRectBase& operator=(const LLSD& sd)
	{
		setValue(sd);
		return *this;
	}

	void setValue(const LLSD& sd)
	{
		mLeft = sd[0].asInteger(); 
		mTop = sd[1].asInteger();
		mRight = sd[2].asInteger();
		mBottom = sd[3].asInteger();
	}

	LLSD getValue() const
	{
		LLSD ret;
		ret[0] = mLeft;
		ret[1] = mTop;
		ret[2] = mRight;
		ret[3] = mBottom;
		return ret;
	}

	// Note: follows GL_QUAD conventions: the top and right edges are not considered part of the rect
	BOOL		pointInRect(const Type x, const Type y) const
	{
		return  mLeft <= x && x < mRight &&
				mBottom <= y && y < mTop;
	}

	//// Note: follows GL_QUAD conventions: the top and right edges are not considered part of the rect
	BOOL		localPointInRect(const Type x, const Type y) const
	{
		return  0 <= x && x < getWidth() &&
				0 <= y && y < getHeight();
	}

	void		clampPointToRect(Type& x, Type& y)
	{
		x = llclamp(x, mLeft, mRight);
		y = llclamp(y, mBottom, mTop);
	}

	void		clipPointToRect(const Type start_x, const Type start_y, Type& end_x, Type& end_y)
	{
		if (!pointInRect(start_x, start_y))
		{
			return;
		}
		Type clip_x = 0;
		Type clip_y = 0;
		Type delta_x = end_x - start_x;
		Type delta_y = end_y - start_y;
		if (end_x > mRight) clip_x = end_x - mRight;
		if (end_x < mLeft) clip_x = end_x - mLeft;
		if (end_y > mTop) clip_y = end_y - mTop;
		if (end_y < mBottom) clip_y = end_y - mBottom;
		// clip_? and delta_? should have same sign, since starting point is in rect
		// so ratios will be positive
		F32 ratio_x = ((F32)clip_x / (F32)delta_x);
		F32 ratio_y = ((F32)clip_y / (F32)delta_y);
		if (ratio_x > ratio_y)
		{
			// clip along x direction
			end_x -= (Type)(clip_x);
			end_y -= (Type)(delta_y * ratio_x);
		}
		else
		{
			// clip along y direction
			end_x -= (Type)(delta_x * ratio_y);
			end_y -= (Type)clip_y;
		}
	}

	// Note: Does NOT follow GL_QUAD conventions: the top and right edges ARE considered part of the rect
	// returns TRUE if any part of rect is is inside this LLRect
	BOOL		rectInRect(const LLRectBase* rect) const
	{
		return mLeft <= rect->mRight && rect->mLeft <= mRight && 
			   mBottom <= rect->mTop && rect->mBottom <= mTop ;
	}

	void		set(Type left, Type top, Type right, Type bottom)
	{
		mLeft = left;
		mTop = top;
		mRight = right;
		mBottom = bottom;
	}

	// Note: follows GL_QUAD conventions: the top and right edges are not considered part of the rect
	void		setOriginAndSize( Type left, Type bottom, Type width, Type height)
	{
		mLeft = left;
		mTop = bottom + height;
		mRight = left + width;
		mBottom = bottom;
	}

	// Note: follows GL_QUAD conventions: the top and right edges are not considered part of the rect
	void		setLeftTopAndSize( Type left, Type top, Type width, Type height)
	{
		mLeft = left;
		mTop = top;
		mRight = left + width;
		mBottom = top - height;
	}

	void setCenterAndSize(Type x, Type y, Type width, Type height)
	{
		mLeft = x - width/2;
		mTop = y + height/2;
		mRight = x + width/2;
		mBottom = y - height/2;
	}


	void		translate(Type horiz, Type vertical)
	{
		mLeft += horiz;
		mRight += horiz;
		mTop += vertical;
		mBottom += vertical;
	}

	void	stretch( Type dx, Type dy)
	{
		mLeft -= dx;
		mRight += dx;
		mTop += dy;
		mBottom -= dy;
		makeValid();
	}
	
	void	stretch( Type delta )
	{
		stretch(delta, delta);
		
	}
	
	void  makeValid()
	{
		mLeft = llmin(mLeft, mRight);
		mBottom = llmin(mBottom, mTop);
	}
	
	void unionWith(const LLRectBase &other)
	{
		mLeft = llmin(mLeft, other.mLeft);
		mRight = llmax(mRight, other.mRight);
		mBottom = llmin(mBottom, other.mBottom);
		mTop = llmax(mTop, other.mTop);
	}

	void intersectWith(const LLRectBase &other)
	{
		mLeft = llmax(mLeft, other.mLeft);
		mRight = llmin(mRight, other.mRight);
		mBottom = llmax(mBottom, other.mBottom);
		mTop = llmin(mTop, other.mTop);
		if (mLeft > mRight)
		{
			mLeft = mRight;
		}
		if (mBottom > mTop)
		{
			mBottom = mTop;
		}
	}

	friend std::ostream &operator<<(std::ostream &s, const LLRectBase &rect)
	{
		s << "{ L " << rect.mLeft << " B " << rect.mBottom
			<< " W " << rect.getWidth() << " H " << rect.getHeight() << " }";
		return s;
	}

	bool operator==(const LLRectBase &b)
	{
		return ((mLeft == b.mLeft) &&
				(mTop == b.mTop) &&
				(mRight == b.mRight) &&
				(mBottom == b.mBottom));
	}

	bool operator!=(const LLRectBase &b)
	{
		return ((mLeft != b.mLeft) ||
				(mTop != b.mTop) ||
				(mRight != b.mRight) ||
				(mBottom != b.mBottom));
	}

	static LLRectBase<Type> null;
};

template <class Type> LLRectBase<Type> LLRectBase<Type>::null(0,0,0,0);

typedef LLRectBase<S32> LLRect;
typedef LLRectBase<F32> LLRectf;

#endif
