/** 
 * @file llrect.h
 * @brief A rectangle in GL coordinates, with bottom,left = 0,0
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	
	friend const LLRectBase& operator|=(LLRectBase &a, const LLRectBase &b)	// Return rect including a & b
	{
		a.mLeft = llmin(a.mLeft, b.mLeft);
		a.mRight = llmax(a.mRight, b.mRight);
		a.mBottom = llmin(a.mBottom, b.mBottom);
		a.mTop = llmax(a.mTop, b.mTop);
		return a;
	}

	friend LLRectBase operator|(const LLRectBase &a, const LLRectBase &b)	// Return rect including a & b
	{
		LLRectBase<Type> result;
		result.mLeft = llmin(a.mLeft, b.mLeft);
		result.mRight = llmax(a.mRight, b.mRight);
		result.mBottom = llmin(a.mBottom, b.mBottom);
		result.mTop = llmax(a.mTop, b.mTop);
		return result;
	}

	friend const LLRectBase& operator&=(LLRectBase &a, const LLRectBase &b)	// set a to rect where a intersects b
	{
		a.mLeft = llmax(a.mLeft, b.mLeft);
		a.mRight = llmin(a.mRight, b.mRight);
		a.mBottom = llmax(a.mBottom, b.mBottom);
		a.mTop = llmin(a.mTop, b.mTop);
		if (a.mLeft > a.mRight)
		{
			a.mLeft = a.mRight;
		}
		if (a.mBottom > a.mTop)
		{
			a.mBottom = a.mTop;
		}
		return a;
	}

	friend LLRectBase operator&(const LLRectBase &a, const LLRectBase &b)	// Return rect where a intersects b
	{
		LLRectBase result = a;
		result &= b;
		return result;
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
