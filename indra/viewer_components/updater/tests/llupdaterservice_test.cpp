/**
 * @file   llupdaterservice_test.cpp
 * @brief  Tests of llupdaterservice.cpp.
 * 
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "../llupdaterservice.h"

#include "../../../test/lltut.h"
//#define DEBUG_ON
#include "../../../test/debug.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llupdaterservice_data
    {
		llupdaterservice_data() :
            pumps(LLEventPumps::instance())
		{}
		LLEventPumps& pumps;
	};

    typedef test_group<llupdaterservice_data> llupdaterservice_group;
    typedef llviewerlogin_group::object llupdaterservice_object;
    llupdaterservice_group llupdaterservicegrp("LLUpdaterService");

    template<> template<>
    void llupdateservice_object::test<1>()
    {
        DEBUG;
		ensure_equals("Dummy", "true", "true");
	}
}
