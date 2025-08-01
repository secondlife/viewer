/**
 * @file llwtextureinfo_test.cpp
 * @author Si & Gabriel
 * @date 2009-03-30
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

// Precompiled header: almost always required for newview cpp files
#include "../llviewerprecompiledheaders.h"
// Class to test
#include "../lltextureinfo.h"
// Dependencies
#include "../lltextureinfodetails.cpp"

// Tut header
#include "../test/lldoctest.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes:
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.


// End Stubbing
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------

TEST_SUITE("LLTectureInfo") {

struct textureinfo_test
{

        // Constructor and destructor of the test wrapper
        textureinfo_test()
        {
        
};

TEST_CASE_FIXTURE(textureinfo_test, "test_1")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);
        CHECK_MESSAGE(true, "have we crashed?");
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_2")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
        ensure(!tex_info.has(nonExistant));
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_3")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestStartTime(id, 200);

        ensure_equals(tex_info.getRequestStartTime(id), 200);
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_4")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
        ensure_equals(tex_info.getRequestStartTime(nonExistant), 0);
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_5")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
        ensure_equals(tex_info.getRequestCompleteTime(nonExistant), 0);
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_6")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestSize(id, 600);

        ensure_equals(tex_info.getRequestSize(id), 600);
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_7")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestType(id, LLTextureInfoDetails::REQUEST_TYPE_HTTP);

        ensure_equals(tex_info.getRequestType(id), LLTextureInfoDetails::REQUEST_TYPE_HTTP);
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_8")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestType(id, LLTextureInfoDetails::REQUEST_TYPE_UDP);

        ensure_equals(tex_info.getRequestType(id), LLTextureInfoDetails::REQUEST_TYPE_UDP);
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_9")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestOffset(id, 1234);

        ensure_equals(tex_info.getRequestOffset(id), 1234);
    
}

TEST_CASE_FIXTURE(textureinfo_test, "test_10")
{

        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        S32 requestStartTimeOne = 200;
        S32 requestEndTimeOne = 400;
        S32 requestSizeOne = 1024;
        S32 requestSizeOneBits = requestSizeOne * 8;
        LLUUID id1("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestStartTime(id1, requestStartTimeOne);
        tex_info.setRequestSize(id1, requestSizeOne);
        tex_info.setRequestType(id1, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
        tex_info.setRequestCompleteTimeAndLog(id1, requestEndTimeOne);

        U32 requestStartTimeTwo = 100;
        U32 requestEndTimeTwo = 500;
        U32 requestSizeTwo = 2048;
        S32 requestSizeTwoBits = requestSizeTwo * 8;
        LLUUID id2("10e65d70-46fd-429f-841a-bf698e9424d4");
        tex_info.setRequestStartTime(id2, requestStartTimeTwo);
        tex_info.setRequestSize(id2, requestSizeTwo);
        tex_info.setRequestType(id2, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
        tex_info.setRequestCompleteTimeAndLog(id2, requestEndTimeTwo);

        S32 averageBitRate = ((requestSizeOneBits/(requestEndTimeOne - requestStartTimeOne)) +
                            (requestSizeTwoBits/(requestEndTimeTwo - requestStartTimeTwo))) / 2;

        S32 totalBytes = requestSizeOne + requestSizeTwo;

        LLSD results = tex_info.getAverages();
        ensure_equals("is average bits per second correct", results["bits_per_second"].asInteger(), averageBitRate);
        ensure_equals("is total bytes is correct", results["bytes_downloaded"].asInteger(), totalBytes);
        ensure_equals("is transport correct", results["transport"].asString(), std::string("HTTP"));
    
}

} // TEST_SUITE
