/**
 * @file   llptrto.cpp
 * @author Nat Goodspeed
 * @date   2008-08-20
 * @brief  Test for llptrto.h
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "llptrto.h"
// STL headers
// std headers
// external library headers
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
// other Linden headers
#include "llmemory.h"

// a refcounted class
class RCFoo: public LLRefCount
{
public:
    RCFoo() {}
};

// a refcounted subclass
class RCSubFoo: public RCFoo
{
public:
    RCSubFoo() {}
};

// a refcounted class using the other refcount base class
class TSRCFoo: public LLThreadSafeRefCount
{
public:
    TSRCFoo() {}
};

// a non-refcounted class
class Bar
{
public:
    Bar() {}
};

// a non-refcounted subclass
class SubBar: public Bar
{
public:
    SubBar() {}
};

int main(int argc, char *argv[])
{
    // test LLPtrTo<>
    BOOST_STATIC_ASSERT((boost::is_same<LLPtrTo<RCFoo>::type, LLPointer<RCFoo> >::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLPtrTo<RCSubFoo>::type, LLPointer<RCSubFoo> >::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLPtrTo<TSRCFoo>::type, LLPointer<TSRCFoo> >::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLPtrTo<Bar>::type, Bar*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLPtrTo<SubBar>::type, SubBar*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLPtrTo<int>::type, int*>::value));

    // Test LLRemovePointer<>. Note that we remove both pointer variants from
    // each kind of type, regardless of whether the variant makes sense.
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer<RCFoo*>::type, RCFoo>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer< LLPointer<RCFoo> >::type, RCFoo>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer<RCSubFoo*>::type, RCSubFoo>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer< LLPointer<RCSubFoo> >::type, RCSubFoo>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer<TSRCFoo*>::type, TSRCFoo>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer< LLPointer<TSRCFoo> >::type, TSRCFoo>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer<Bar*>::type, Bar>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer< LLPointer<Bar> >::type, Bar>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer<SubBar*>::type, SubBar>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer< LLPointer<SubBar> >::type, SubBar>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer<int*>::type, int>::value));
    BOOST_STATIC_ASSERT((boost::is_same<LLRemovePointer< LLPointer<int> >::type, int>::value));

    return 0;
}
