/**
 * @file   llallocator_test.cpp
 * @author Brad Kittenbrink
 * @date   2008-02-
 * @brief  Test for llallocator.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2009-2009, Linden Research, Inc.
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
