/**
 * @file lltexturestatsuploader_test.cpp
 * @author Si
 * @date 2009-05-27
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
#include "../lltexturestatsuploader.h"
// Dependencies

// Tut header
#include "../test/lldoctest.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes:
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.

#include "boost/intrusive_ptr.hpp"
void boost::intrusive_ptr_add_ref(LLCurl::Responder*){}
void boost::intrusive_ptr_release(LLCurl::Responder* p){}
const F32 HTTP_REQUEST_EXPIRY_SECS = 0.0f;

static std::string most_recent_url;
static LLSD most_recent_body;

void LLHTTPClient::post(
        const std::string& url,
        const LLSD& body,
        ResponderPtr,
        const LLSD& headers,
        const F32 timeout)
{
    // set some sensor code
    most_recent_url = url;
    most_recent_body = body;
    return;
}

// End Stubbing
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------

TEST_SUITE("LLTextureStatsUploader") {

struct texturestatsuploader_test
{

        // Constructor and destructor of the test wrapper
        texturestatsuploader_test()
        {
            most_recent_url = "some sort of default text that should never match anything the tests are expecting!";
            LLSD blank_llsd;
            most_recent_body = blank_llsd;
        
};

TEST_CASE_FIXTURE(texturestatsuploader_test, "test_1")
{

        LLTextureStatsUploader tsu;
        LL_INFOS() << &tsu << LL_ENDL;
        CHECK_MESSAGE(true, "have we crashed?");
    
}

TEST_CASE_FIXTURE(texturestatsuploader_test, "test_2")
{

        LLTextureStatsUploader tsu;
        std::string url = "http://blahblahblah";
        LLSD texture_stats;
        tsu.uploadStatsToSimulator(url, texture_stats);
        CHECK_MESSAGE(most_recent_url == url, "did the right url get called?");
        CHECK_MESSAGE(most_recent_body == texture_stats, "did the right body get sent?");
    
}

} // TEST_SUITE
