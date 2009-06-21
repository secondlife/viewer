/**
 * @file   llptrto.cpp
 * @author Nat Goodspeed
 * @date   2008-08-20
 * @brief  Test for llptrto.h
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
