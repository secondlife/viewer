/** 
 * @file lllazyvalue.h
 * @brief generic functor/value abstraction for lazy evaluation of a value
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LAZY_VALUE_H
#define LL_LAZY_VALUE_H

#include <boost/function.hpp>

// Holds on to a value of type T *or* calls a functor to generate a value of type T
template<typename T>
class LLLazyValue
{
public:
	typedef typename boost::add_reference<typename boost::add_const<T>::type>::type	T_const_ref;
	typedef typename boost::function<T_const_ref (void)>							function_type;

public:
	LLLazyValue(const function_type& value) 
	:	mValueGetter(value)
	{} 
	LLLazyValue(T_const_ref value)
	:	mValue(value)
	{}
	LLLazyValue()
	:	mValue()
	{}

	void set(const LLLazyValue& val)
	{
		mValueGetter = val.mValueGetter;
	}

	void set(T_const_ref val)
	{
		mValue = val;
		mValueGetter = NULL;
	}

	T_const_ref get() const
	{
		if (!mValueGetter.empty())
		{
			return mValueGetter();
		}
		return mValue;
	}

private:
	function_type	mValueGetter;
	T				mValue;
};

#endif // LL_LAZY_VALUE_H
