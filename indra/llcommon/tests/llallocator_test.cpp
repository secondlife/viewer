/**
 * @file   llallocator_test.cpp
 * @author Brad Kittenbrink
 * @date   2008-02-
 * @brief  Test for llallocator.cpp.
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

#include "../llallocator.h"
#include "../test/lltut.h"

namespace tut
{
    struct llallocator_data
    {
        LLAllocator llallocator;
    };
    typedef test_group<llallocator_data> factory;
    typedef factory::object object;
}
namespace
{
        tut::factory llallocator_test_factory("LLAllocator");
}

namespace tut
{
    template<> template<>
    void object::test<1>()
    {
        llallocator.setProfilingEnabled(false);
        ensure("Profiler disable", !llallocator.isProfiling());
    }

#if LL_USE_TCMALLOC
    template<> template<>
    void object::test<2>()
    {
        llallocator.setProfilingEnabled(true);
        ensure("Profiler enable", llallocator.isProfiling());
    }

    template <> template <>
    void object::test<3>()
    {
        llallocator.setProfilingEnabled(true);

        char * test_alloc = new char[1024];

        llallocator.getProfile();

        delete [] test_alloc;

        llallocator.getProfile();        

        // *NOTE - this test isn't ensuring anything right now other than no
        // exceptions are thrown.
    }
#endif // LL_USE_TCMALLOC
};
