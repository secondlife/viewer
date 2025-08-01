/**
 * @file llmediaentry_test.cpp
 * @brief llmediaentry unit tests
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "../test/lldoctest.h"

#include <boost/lexical_cast.hpp>

#include "llstring.h"
#include "llsdutil.h"
#include "llsdserialize.h"

#include "../llmediaentry.h"
#include "indra_constants.h"

#define DEFAULT_MEDIA_ENTRY "<llsd>\n\
 <map>\n\
 <key>alt_image_enable</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_loop</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_play</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_scale</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_zoom</key>\n\
 <boolean>0</boolean>\n\
 <key>controls</key>\n\
 <integer>0</integer>\n\
 <key>current_url</key>\n\
 <string />\n\
 <key>first_click_interact</key>\n\
 <boolean>0</boolean>\n\
 <key>height_pixels</key>\n\
 <integer>0</integer>\n\
 <key>home_url</key>\n\
 <string />\n\
 <key>perms_control</key>\n\
 <integer>7</integer>\n\
 <key>perms_interact</key>\n\
 <integer>7</integer>\n\
 <key>whitelist_enable</key>\n\
 <boolean>0</boolean>\n\
 <key>width_pixels</key>\n\
 <integer>0</integer>\n\
 </map>\n\
 </llsd>"

#define EMPTY_MEDIA_ENTRY "<llsd>\n\
 <map>\n\
 <key>alt_image_enable</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_loop</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_play</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_scale</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_zoom</key>\n\
 <boolean>0</boolean>\n\
 <key>controls</key>\n\
 <integer>0</integer>\n\
 <key>current_url</key>\n\
 <string />\n\
 <key>first_click_interact</key>\n\
 <boolean>0</boolean>\n\
 <key>height_pixels</key>\n\
 <integer>0</integer>\n\
 <key>home_url</key>\n\
 <string />\n\
 <key>perms_control</key>\n\
 <integer>0</integer>\n\
 <key>perms_interact</key>\n\
 <integer>0</integer>\n\
 <key>whitelist_enable</key>\n\
 <boolean>0</boolean>\n\
 <key>width_pixels</key>\n\
 <integer>0</integer>\n\
 </map>\n\
 </llsd>"

#define PARTIAL_MEDIA_ENTRY(CURRENT_URL) "<llsd>\n\
 <map>\n\
 <key>alt_image_enable</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_loop</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_play</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_scale</key>\n\
 <boolean>0</boolean>\n\
 <key>auto_zoom</key>\n\
 <boolean>0</boolean>\n\
 <key>controls</key>\n\
 <integer>0</integer>\n\
 <key>current_url</key>\n\
 <string>" CURRENT_URL "</string>\n\
 <key>first_click_interact</key>\n\
 <boolean>0</boolean>\n\
 <key>height_pixels</key>\n\
 <integer>0</integer>\n\
 <key>home_url</key>\n\
 <string />\n\
 <key>perms_control</key>\n\
 <integer>0</integer>\n\
 <key>perms_interact</key>\n\
 <integer>0</integer>\n\
 <key>whitelist_enable</key>\n\
 <boolean>0</boolean>\n\
 <key>width_pixels</key>\n\
 <integer>0</integer>\n\
 </map>\n\
 </llsd>"

TEST_SUITE("LLMediaEntry") {

struct MediaEntry_test
{

        MediaEntry_test() {
            emptyMediaEntryStr = EMPTY_MEDIA_ENTRY;
            std::istringstream e(EMPTY_MEDIA_ENTRY);
            LLSDSerialize::fromXML(emptyMediaEntryLLSD, e);
            defaultMediaEntryStr = DEFAULT_MEDIA_ENTRY;
            std::istringstream d(DEFAULT_MEDIA_ENTRY);
            LLSDSerialize::fromXML(defaultMediaEntryLLSD, d);
        
};

TEST_CASE_FIXTURE(MediaEntry_test, "test_1")
{

        set_test_name("Test LLMediaEntry Instantiation");
        LLMediaEntry entry;
        ensure_llsd_equals(get_test_name() + " failed", defaultMediaEntryLLSD, entry.asLLSD());
    
}

TEST_CASE_FIXTURE(MediaEntry_test, "test_2")
{

        set_test_name("Test LLMediaEntry Instantiation from LLSD");
        LLMediaEntry entry;
        LLSD sd;
        entry.fromLLSD(sd);
        ensure_llsd_equals(get_test_name() + " failed", emptyMediaEntryLLSD, entry.asLLSD());
    
}

TEST_CASE_FIXTURE(MediaEntry_test, "test_3")
{

        set_test_name("Test LLMediaEntry Partial Instantiation from LLSD");
        LLMediaEntry entry;
        LLSD sd;
        sd[LLMediaEntry::CURRENT_URL_KEY] = "http://www.example.com";
        entry.fromLLSD(sd);
        LLSD golden;
        std::istringstream p(PARTIAL_MEDIA_ENTRY("http://www.example.com"));
        LLSDSerialize::fromXML(golden,p);
        ensure_llsd_equals(get_test_name() + " failed", golden, entry.asLLSD());
    
}

TEST_CASE_FIXTURE(MediaEntry_test, "test_4")
{

        set_test_name("Test LLMediaEntry::asLLSD()");
        LLMediaEntry entry;
        LLSD sd;
        // Put some cruft in the LLSD
        sd[LLMediaEntry::CURRENT_URL_KEY] = "http://www.example.com";
        LLSD whitelist;
        whitelist.append("*.example.com");
        sd[LLMediaEntry::WHITELIST_KEY] = whitelist;
        entry.asLLSD(sd);
        ensure_llsd_equals(get_test_name() + " failed", defaultMediaEntryLLSD, sd);
    
}

} // TEST_SUITE
