/** 
 * @file llrandom_test.cpp
 * @author Phoenix
 * @date 2007-01-25
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "../test/lltut.h"

#include "../llrand.h"


namespace tut
{
    struct random
    {
    };

    typedef test_group<random> random_t;
    typedef random_t::object random_object_t;
    tut::random_t tut_random("LLSeedRand");

    template<> template<>
    void random_object_t::test<1>()
    {
        F32 number = 0.0f;
        for(S32 ii = 0; ii < 100000; ++ii)
        {
            number = ll_frand();
            ensure("frand >= 0", (number >= 0.0f));
            ensure("frand < 1", (number < 1.0f));
        }
    }

    template<> template<>
    void random_object_t::test<2>()
    {
        F64 number = 0.0f;
        for(S32 ii = 0; ii < 100000; ++ii)
        {
            number = ll_drand();
            ensure("drand >= 0", (number >= 0.0));
            ensure("drand < 1", (number < 1.0));
        }
    }

    template<> template<>
    void random_object_t::test<3>()
    {
        F32 number = 0.0f;
        for(S32 ii = 0; ii < 100000; ++ii)
        {
            number = ll_frand(2.0f) - 1.0f;
            ensure("frand >= 0", (number >= -1.0f));
            ensure("frand < 1", (number <= 1.0f));
        }
    }

    template<> template<>
    void random_object_t::test<4>()
    {
        F32 number = 0.0f;
        for(S32 ii = 0; ii < 100000; ++ii)
        {
            number = ll_frand(-7.0);
            ensure("drand <= 0", (number <= 0.0));
            ensure("drand > -7", (number > -7.0));
        }
    }

    template<> template<>
    void random_object_t::test<5>()
    {
        F64 number = 0.0f;
        for(S32 ii = 0; ii < 100000; ++ii)
        {
            number = ll_drand(-2.0);
            ensure("drand <= 0", (number <= 0.0));
            ensure("drand > -2", (number > -2.0));
        }
    }

    template<> template<>
    void random_object_t::test<6>()
    {
        S32 number = 0;
        for(S32 ii = 0; ii < 100000; ++ii)
        {
            number = ll_rand(100);
            ensure("rand >= 0", (number >= 0));
            ensure("rand < 100", (number < 100));
        }
    }

    template<> template<>
    void random_object_t::test<7>()
    {
        S32 number = 0;
        for(S32 ii = 0; ii < 100000; ++ii)
        {
            number = ll_rand(-127);
            ensure("rand <= 0", (number <= 0));
            ensure("rand > -127", (number > -127));
        }
    }
}
