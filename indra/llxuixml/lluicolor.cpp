/** 
 * @file lluicolor.cpp
 * @brief brief LLUIColor class implementation file
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
