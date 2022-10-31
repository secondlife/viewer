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
#include "../test/lltut.h"

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

namespace tut
{
    // Test wrapper declarations
    struct texturestatsuploader_test
    {
        // Constructor and destructor of the test wrapper
        texturestatsuploader_test()
        {
            most_recent_url = "some sort of default text that should never match anything the tests are expecting!";
            LLSD blank_llsd;
            most_recent_body = blank_llsd;
        }
        ~texturestatsuploader_test()
        {
        }
    };

    // Tut templating thingamagic: test group, object and test instance
    typedef test_group<texturestatsuploader_test> texturestatsuploader_t;
    typedef texturestatsuploader_t::object texturestatsuploader_object_t;
    tut::texturestatsuploader_t tut_texturestatsuploader("LLTextureStatsUploader");

    
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
    void texturestatsuploader_object_t::test<1>()
    {
        LLTextureStatsUploader tsu;
        LL_INFOS() << &tsu << LL_ENDL;
        ensure("have we crashed?", true);
    }

    // does it call out to the provided url if we ask it to?
    template<> template<>
    void texturestatsuploader_object_t::test<2>()
    {   
        LLTextureStatsUploader tsu;
        std::string url = "http://blahblahblah";
        LLSD texture_stats;
        tsu.uploadStatsToSimulator(url, texture_stats);
        ensure_equals("did the right url get called?", most_recent_url, url);
        ensure_equals("did the right body get sent?", most_recent_body, texture_stats);
    }

    // does it not call out to the provided url if we send it an ungranted cap?
    template<> template<>
    void texturestatsuploader_object_t::test<3>()
    {   
        LLTextureStatsUploader tsu;

        // this url left intentionally blank to mirror
        // not getting a cap in the caller.
        std::string url_for_ungranted_cap = ""; 
                              
        LLSD texture_stats;
        std::string most_recent_url_before_test = most_recent_url;
        tsu.uploadStatsToSimulator(url_for_ungranted_cap, texture_stats);

        ensure_equals("hopefully no url got called!", most_recent_url, most_recent_url_before_test);
    }

    // does it call out if the data is empty?
    // should it even do that?
}

