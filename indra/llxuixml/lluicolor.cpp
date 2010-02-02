/** 
 * @file lluicolor.cpp
 * @brief brief LLUIColor class implementation file
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "lluicolor.h"

LLUIColor::LLUIColor()
	:mColorPtr(NULL)
{
}


LLUIColor::LLUIColor(const LLColor4& color)
:	mColor(color), 
	mColorPtr(NULL)
{
}

LLUIColor::LLUIColor(const LLUIColor* color)
:	mColorPtr(color)
{
}

void LLUIColor::set(const LLColor4& color)
{
	mColor = color;
	mColorPtr = NULL;
}

void LLUIColor::set(const LLUIColor* color)
{
	mColorPtr = color;
}

const LLColor4& LLUIColor::get() const
{
	return (mColorPtr == NULL ? mColor : mColorPtr->get());
}

LLUIColor::operator const LLColor4& () const
{
	return get();
}

const LLColor4& LLUIColor::operator()() const
{
	return get();
}

bool LLUIColor::isReference() const
{
	return mColorPtr != NULL;
}

namespace LLInitParam
{
	// used to detect equivalence with default values on export
	bool ParamCompare<LLUIColor, false>::equals(const LLUIColor &a, const LLUIColor &b)
	{
		// do not detect value equivalence, treat pointers to colors as distinct from color values
		return (a.mColorPtr == NULL && b.mColorPtr == NULL ? a.mColor == b.mColor : a.mColorPtr == b.mColorPtr);
	}
}
