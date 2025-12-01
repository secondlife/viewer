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
#include <type_traits>
// std headers
// external library headers
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
    static_assert((std::is_same_v<LLPtrTo<RCFoo>::type, LLPointer<RCFoo> >));
    static_assert((std::is_same_v<LLPtrTo<RCSubFoo>::type, LLPointer<RCSubFoo> >));
    static_assert((std::is_same_v<LLPtrTo<TSRCFoo>::type, LLPointer<TSRCFoo> >));
    static_assert((std::is_same_v<LLPtrTo<Bar>::type, Bar*>));
    static_assert((std::is_same_v<LLPtrTo<SubBar>::type, SubBar*>));
    static_assert((std::is_same_v<LLPtrTo<int>::type, int*>));

    // Test LLRemovePointer<>. Note that we remove both pointer variants from
    // each kind of type, regardless of whether the variant makes sense.
    static_assert((std::is_same_v<LLRemovePointer<RCFoo*>::type, RCFoo>));
    static_assert((std::is_same_v<LLRemovePointer< LLPointer<RCFoo> >::type, RCFoo>));
    static_assert((std::is_same_v<LLRemovePointer<RCSubFoo*>::type, RCSubFoo>));
    static_assert((std::is_same_v<LLRemovePointer< LLPointer<RCSubFoo> >::type, RCSubFoo>));
    static_assert((std::is_same_v<LLRemovePointer<TSRCFoo*>::type, TSRCFoo>));
    static_assert((std::is_same_v<LLRemovePointer< LLPointer<TSRCFoo> >::type, TSRCFoo>));
    static_assert((std::is_same_v<LLRemovePointer<Bar*>::type, Bar>));
    static_assert((std::is_same_v<LLRemovePointer< LLPointer<Bar> >::type, Bar>));
    static_assert((std::is_same_v<LLRemovePointer<SubBar*>::type, SubBar>));
    static_assert((std::is_same_v<LLRemovePointer< LLPointer<SubBar> >::type, SubBar>));
    static_assert((std::is_same_v<LLRemovePointer<int*>::type, int>));
    static_assert((std::is_same_v<LLRemovePointer< LLPointer<int> >::type, int>));

    return 0;
}
