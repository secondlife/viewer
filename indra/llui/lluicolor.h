/** 
 * @file lluicolor.h
 * @brief brief LLUIColor class header file
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

#ifndef LL_LLUICOLOR_H_
#define LL_LLUICOLOR_H_

#include "v4color.h"

namespace LLInitParam
{
	template<typename T, bool>
	struct ParamCompare;
}

class LLUIColor
{
public:
	LLUIColor();
	LLUIColor(const LLColor4& color);
	LLUIColor(const LLUIColor* color);

	void set(const LLColor4& color);
	void set(const LLUIColor* color);

	const LLColor4& get() const;

	operator const LLColor4& () const;
	const LLColor4& operator()() const;

	bool isReference() const;

private:
	friend struct LLInitParam::ParamCompare<LLUIColor, false>;

	const LLUIColor* mColorPtr;
	LLColor4 mColor;
};

namespace LLInitParam
{
	template<>
	struct ParamCompare<LLUIColor, false>
	{
		static bool equals(const LLUIColor& a, const LLUIColor& b);
	};
}

#endif
