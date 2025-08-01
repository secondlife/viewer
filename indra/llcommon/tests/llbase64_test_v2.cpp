/**
 * @file llbase64_test.cpp
 * @author James Cook
 * @date 2007-02-04
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

#include "../llbase64.h"
#include "../lluuid.h"

#include "../test/lldoctest.h"

TEST_SUITE("LLBase64") {

TEST_CASE("test_1")
{

        std::string result;

        result = LLBase64::encode(NULL, 0);
        CHECK_MESSAGE((result == "", "encode nothing") );

        LLUUID nothing;
        result = LLBase64::encode(&nothing.mData[0], UUID_BYTES);
        CHECK_MESSAGE((result == "AAAAAAAAAAAAAAAAAAAAAA==", "encode blank uuid") );

        LLUUID id("526a1e07-a19d-baed-84c4-ff08a488d15e");
        result = LLBase64::encode(&id.mData[0], UUID_BYTES);
        CHECK_MESSAGE((result == "UmoeB6Gduu2ExP8IpIjRXg==", "encode random uuid") );

    
}

TEST_CASE("test_2")
{

        std::string result;

        U8 blob[40] = { 115, 223, 172, 255, 140, 70, 49, 125, 236, 155, 45, 199, 101, 17, 164, 131, 230, 19, 80, 64, 112, 53, 135, 98, 237, 12, 26, 72, 126, 14, 145, 143, 118, 196, 11, 177, 132, 169, 195, 134 
}

} // TEST_SUITE

