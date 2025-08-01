/**
 * @file llinventoryparcel_tut.cpp
 * @author Moss
 * @date 2007-04-17
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

#include <string>

#include "linden_common.h"

#include "../llparcel.h"

#include "../test/lldoctest.h"

TEST_SUITE("LLInventoryParcel") {

TEST_CASE("test_1")
{

        for (S32 i=0; i<LLParcel::C_COUNT; ++i)
        {
            const std::string& catstring =  LLParcel::getCategoryString(LLParcel::ECategory(i));
            CHECK_MESSAGE(!catstring.empty(, "LLParcel::getCategoryString(i)"));

            const std::string& catuistring =  LLParcel::getCategoryUIString(LLParcel::ECategory(i));
            CHECK_MESSAGE(!catuistring.empty(, "LLParcel::getCategoryUIString(i)"));

            ensure_equals("LLParcel::ECategory mapping of string back to enum", LLParcel::getCategoryFromString(catstring), i);
            ensure_equals("LLParcel::ECategory mapping of uistring back to enum", LLParcel::getCategoryFromUIString(catuistring), i);
        
}

} // TEST_SUITE

