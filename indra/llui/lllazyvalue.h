/** 
 * @file lllazyvalue.h
 * @brief generic functor/value abstraction for lazy evaluation of a value
 * parsing construction parameters from xml and LLSD
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LAZY_VALUE_H
#define LL_LAZY_VALUE_H

#include <boost/function.hpp>

// Holds on to a value of type T *or* calls a functor to generate a value of type T
template<typename T>
class LLLazyValue
{
public:
    typedef typename boost::add_reference<typename boost::add_const<T>::type>::type T_const_ref;
    typedef typename boost::function<T_const_ref (void)>                            function_type;

public:
    LLLazyValue(const function_type& value) 
    :   mValueGetter(value)
    {} 
    LLLazyValue(T_const_ref value)
    :   mValue(value)
    {}
    LLLazyValue()
    :   mValue()
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

    bool isUsingFunction() const
    {
        return mValueGetter != NULL;
    }

private:
    function_type   mValueGetter;
    T               mValue;
};

#endif // LL_LAZY_VALUE_H
