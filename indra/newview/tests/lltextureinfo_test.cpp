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
#include "../test/lltut.h"

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

namespace tut
{
    // Test wrapper declarations
    struct textureinfo_test
    {
        // Constructor and destructor of the test wrapper
        textureinfo_test()
        {
        }
        ~textureinfo_test()
        {
        }
    };

    // Tut templating thingamagic: test group, object and test instance
    typedef test_group<textureinfo_test> textureinfo_t;
    typedef textureinfo_t::object textureinfo_object_t;
    tut::textureinfo_t tut_textureinfo("LLTectureInfo");

    
    // ---------------------------------------------------------------------------------------
    // Test functions
    // Notes:
    // * Test as many as you possibly can without requiring a full blown simulation of everything
    // * The tests are executed in sequence so the test instance state may change between calls
    // * Remember that you cannot test private methods with tut
    // ---------------------------------------------------------------------------------------

    // ---------------------------------------------------------------------------------------
    // Test the LLTextureInfo
    // ---------------------------------------------------------------------------------------


    // Test instantiation
    template<> template<>
    void textureinfo_object_t::test<1>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);
        ensure("have we crashed?", true);
    }

    // Check lltextureinfo does not contain UUIDs we haven't added
    template<> template<>
    void textureinfo_object_t::test<2>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
        ensure(!tex_info.has(nonExistant));
    }

    // Check we can add a request time for a texture
    template<> template<>
    void textureinfo_object_t::test<3>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestStartTime(id, 200);

        ensure_equals(tex_info.getRequestStartTime(id), 200);
    }

    // Check time for non-existant texture
    template<> template<>
    void textureinfo_object_t::test<4>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
        ensure_equals(tex_info.getRequestStartTime(nonExistant), 0);
    }

    // Check download complete time for non existant texture
    template<> template<>
    void textureinfo_object_t::test<5>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID nonExistant("3a0efa3b-84dc-4e17-9b8c-79ea028850c1");
        ensure_equals(tex_info.getRequestCompleteTime(nonExistant), 0);
    }

    // requested size is passed in correctly
    template<> template<>
    void textureinfo_object_t::test<6>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestSize(id, 600);

        ensure_equals(tex_info.getRequestSize(id), 600);
    }

    // transport type is recorded correctly (http)
    template<> template<>
    void textureinfo_object_t::test<7>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestType(id, LLTextureInfoDetails::REQUEST_TYPE_HTTP);

        ensure_equals(tex_info.getRequestType(id), LLTextureInfoDetails::REQUEST_TYPE_HTTP);
    }

    // transport type is recorded correctly (udp)
    template<> template<>
    void textureinfo_object_t::test<8>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestType(id, LLTextureInfoDetails::REQUEST_TYPE_UDP);

        ensure_equals(tex_info.getRequestType(id), LLTextureInfoDetails::REQUEST_TYPE_UDP);
    }

    // request offset is recorded correctly
    template<> template<>
    void textureinfo_object_t::test<9>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        LLUUID id("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestOffset(id, 1234);

        ensure_equals(tex_info.getRequestOffset(id), 1234);
    }

    // ask for averages gives us correct figure
    template<> template<>
    void textureinfo_object_t::test<10>()
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

    // make sure averages cleared when reset is called
    template<> template<>
    void textureinfo_object_t::test<11>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        S32 requestStartTimeOne = 200;
        S32 requestEndTimeOne = 400;
        S32 requestSizeOne = 1024;
        LLUUID id1("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestStartTime(id1, requestStartTimeOne);
        tex_info.setRequestSize(id1, requestSizeOne);
        tex_info.setRequestType(id1, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
        tex_info.setRequestCompleteTimeAndLog(id1, requestEndTimeOne);

        tex_info.getAverages();
        tex_info.reset();
        LLSD results = tex_info.getAverages();
        ensure_equals("is average bits per second correct", results["bits_per_second"].asInteger(), 0);
        ensure_equals("is total bytes is correct", results["bytes_downloaded"].asInteger(), 0);
        ensure_equals("is transport correct", results["transport"].asString(), std::string("NONE"));
    }

    // make sure map item removed when expired
    template<> template<>
    void textureinfo_object_t::test<12>()
    {
        LLTextureInfo tex_info;
        tex_info.setUpLogging(true, true);

        S32 requestStartTimeOne = 200;
        S32 requestEndTimeOne = 400;
        S32 requestSizeOne = 1024;
        LLUUID id1("10e65d70-46fd-429f-841a-bf698e9424d3");
        tex_info.setRequestStartTime(id1, requestStartTimeOne);
        tex_info.setRequestSize(id1, requestSizeOne);
        tex_info.setRequestType(id1, LLTextureInfoDetails::REQUEST_TYPE_HTTP);

        ensure_equals("map item created", tex_info.getTextureInfoMapSize(), 1);

        tex_info.setRequestCompleteTimeAndLog(id1, requestEndTimeOne);

        ensure_equals("map item removed when consumed", tex_info.getTextureInfoMapSize(), 0);
    }
}

