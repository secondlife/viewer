/** 
 * @file lluicolor.h
 * @brief brief LLUIColor class header file
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLUICOLOR_H_
#define LL_LLUICOLOR_H_

#include "v4color.h"
#include <boost/type_traits/integral_constant.hpp> // for boost::false_type

namespace LLInitParam
{
	template<typename T, bool>
	struct ParamCompare;
}

class LLUIColor
{
public:
	LLUIColor();
	LLUIColor(const LLColor4* color);
	LLUIColor(const LLColor4& color);

	void set(const LLColor4& color);
	void set(const LLColor4* color);

	const LLColor4& get() const;

	operator const LLColor4& () const;
	const LLColor4& operator()() const;

	bool isReference() const;

private:
	friend struct LLInitParam::ParamCompare<LLUIColor, false>;

	const LLColor4* mColorPtr;
	LLColor4 mColor;
};

namespace LLInitParam
{
	template<>
	struct ParamCompare<LLUIColor, false>
	{
		static bool equals(const class LLUIColor& a, const class LLUIColor& b);
	};
}

#endif
