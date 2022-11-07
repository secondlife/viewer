/** 
 * @file llprocinfo_test.cpp
 * @brief Tests for the LLProcInfo class.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "../llprocinfo.h"

#include "../test/lltut.h"
#include "../lltimer.h"


static const LLProcInfo::time_type bad_user(289375U), bad_system(275U);


namespace tut
{

struct procinfo_test
{
    procinfo_test()
        {
        }
};

typedef test_group<procinfo_test> procinfo_group_t;
typedef procinfo_group_t::object procinfo_object_t;
tut::procinfo_group_t procinfo_instance("LLProcInfo");


// Basic invocation works
template<> template<>
void procinfo_object_t::test<1>()
{
    LLProcInfo::time_type user(bad_user), system(bad_system);

    set_test_name("getCPUUsage() basic function");

    LLProcInfo::getCPUUsage(user, system);
    
    ensure_not_equals("getCPUUsage() writes to its user argument", user, bad_user);
    ensure_not_equals("getCPUUsage() writes to its system argument", system, bad_system);
}


// Time increases
template<> template<>
void procinfo_object_t::test<2>()
{
    LLProcInfo::time_type user(bad_user), system(bad_system);
    LLProcInfo::time_type user2(bad_user), system2(bad_system);

    set_test_name("getCPUUsage() increases over time");

    LLProcInfo::getCPUUsage(user, system);
    
    for (int i(0); i < 100000; ++i)
    {
        ms_sleep(0);
    }
    
    LLProcInfo::getCPUUsage(user2, system2);

    ensure_equals("getCPUUsage() user value doesn't decrease over time", user2 >= user, true);
    ensure_equals("getCPUUsage() system value doesn't decrease over time", system2 >= system, true);
}


} // end namespace tut
