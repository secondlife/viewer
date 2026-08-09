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
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"

#include <boost/lexical_cast.hpp>

#include "llstring.h"
#include "llsdutil.h"
#include "llsdserialize.h"

#include "../llmediaentry.h"
#include "indra_constants.h"

#include <sstream>
#include <string>
#include <vector>

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

namespace
{
const char *k_url_ok = "http://www.example.com";
    const char *k_url_too_big = "http://www.example.com.qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";
}

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::set_test_name;

    inline std::string& current_test_name()
    {
        static std::string name;
        return name;
    }

    inline void remember_test_name(const char* name)
    {
        set_test_name(name);
        current_test_name() = name ? name : "";
    }

    inline const std::string& get_test_name()
    {
        return current_test_name();
    }

    struct MediaEntry_test
    {
        MediaEntry_test()
        {
            emptyMediaEntryStr = EMPTY_MEDIA_ENTRY;
            std::istringstream e(EMPTY_MEDIA_ENTRY);
            LLSDSerialize::fromXML(emptyMediaEntryLLSD, e);
            defaultMediaEntryStr = DEFAULT_MEDIA_ENTRY;
            std::istringstream d(DEFAULT_MEDIA_ENTRY);
            LLSDSerialize::fromXML(defaultMediaEntryLLSD, d);
        }

        std::string emptyMediaEntryStr;
        LLSD emptyMediaEntryLLSD;
        std::string defaultMediaEntryStr;
        LLSD defaultMediaEntryLLSD;
    };

    void ensure_llsd_equals(const std::string& msg, const LLSD& expected, const LLSD& actual)
    {
        if (!llsd_equals(expected, actual))
        {
            std::string message = msg;
            message += ": actual: ";
            message += ll_pretty_print_sd(actual);
            message += "\n  expected: ";
            message += ll_pretty_print_sd(expected);
            message += "\n";
            ensure(message, false);
        }
    }

    void ensure_string_equals(const std::string& msg, const std::string& expected, const std::string& actual)
    {
        if (expected != actual)
        {
            std::string message = msg;
            message += ": actual: ";
            message += actual;
            message += "\n  expected: ";
            message += expected;
            message += "\n";
            ensure(message, false);
        }
    }

    void set_whitelist(LLMediaEntry &entry, const char *str)
    {
        std::vector<std::string> tokens;
        LLStringUtil::getTokens(std::string(str), tokens, ",");
        entry.setWhiteList(tokens);
    }

    void whitelist_test(int num, bool enable, const char *whitelist, const char *candidate_url, bool expected_pass)
    {
        std::string message = "Whitelist test " + std::to_string(num);
        LLMediaEntry entry;
        entry.setWhiteListEnable(enable);
        set_whitelist(entry, whitelist);
        bool passed_whitelist = entry.checkCandidateUrl(candidate_url);
        if (passed_whitelist != expected_pass)
        {
            message += " failed: expected ";
            message += expected_pass ? "" : "NOT ";
            message += "to match\nwhitelist = ";
            message += whitelist;
            message += "\ncandidate_url = ";
            message += candidate_url;
        }
        ensure(message, expected_pass == passed_whitelist);
    }

    void whitelist_test(int num, const char *whitelist, const char *candidate_url, bool expected_pass)
    {
        whitelist_test(num, true, whitelist, candidate_url, expected_pass);
    }

    void whitelist_test(int num, const char *whitelist, const char *candidate_url)
    {
        whitelist_test(num, true, whitelist, candidate_url, true);
    }
} // namespace tut

TUT_SUITE("LLMediaEntry")
{
    TUT_CASE("LLMediaEntry::object_test_1")
    {
        using namespace tut;
        MediaEntry_test data;
        remember_test_name("Test LLMediaEntry Instantiation");
        LLMediaEntry entry;
        ensure_llsd_equals(get_test_name() + " failed", data.defaultMediaEntryLLSD, entry.asLLSD());
    }

    TUT_CASE("LLMediaEntry::object_test_2")
    {
        using namespace tut;
        MediaEntry_test data;
        remember_test_name("Test LLMediaEntry Instantiation from LLSD");
        LLMediaEntry entry;
        LLSD sd;
        entry.fromLLSD(sd);
        ensure_llsd_equals(get_test_name() + " failed", data.emptyMediaEntryLLSD, entry.asLLSD());
    }

    TUT_CASE("LLMediaEntry::object_test_3")
    {
        using namespace tut;
        remember_test_name("Test LLMediaEntry Partial Instantiation from LLSD");
        LLMediaEntry entry;
        LLSD sd;
        sd[LLMediaEntry::CURRENT_URL_KEY] = "http://www.example.com";
        entry.fromLLSD(sd);
        LLSD golden;
        std::istringstream p(PARTIAL_MEDIA_ENTRY("http://www.example.com"));
        LLSDSerialize::fromXML(golden, p);
        ensure_llsd_equals(get_test_name() + " failed", golden, entry.asLLSD());
    }

    TUT_CASE("LLMediaEntry::object_test_4")
    {
        using namespace tut;
        MediaEntry_test data;
        remember_test_name("Test LLMediaEntry::asLLSD()");
        LLMediaEntry entry;
        LLSD sd;
        sd[LLMediaEntry::CURRENT_URL_KEY] = "http://www.example.com";
        LLSD whitelist;
        whitelist.append("*.example.com");
        sd[LLMediaEntry::WHITELIST_KEY] = whitelist;
        entry.asLLSD(sd);
        ensure_llsd_equals(get_test_name() + " failed", data.defaultMediaEntryLLSD, sd);
    }

    TUT_CASE("LLMediaEntry::object_test_5")
    {
        using namespace tut;
        MediaEntry_test data;
        remember_test_name("Test LLMediaEntry::asLLSD() -> LLMediaEntry::fromLLSD()");
        LLMediaEntry entry1, entry2;
        std::vector<std::string> whitelist;
        whitelist.push_back("*.example.com");
        entry2.setWhiteList(whitelist);
        LLSD sd;
        entry1.asLLSD(sd);
        entry2.fromLLSD(sd);
        ensure_llsd_equals(get_test_name() + " failed", data.defaultMediaEntryLLSD, entry2.asLLSD());
    }

    TUT_CASE("LLMediaEntry::object_test_6")
    {
        using namespace tut;
        remember_test_name("Test Limits on setting current URL");
        const char *URL_OK = k_url_ok;
        const char *URL_TOO_BIG = k_url_too_big;
        LLMediaEntry entry;
        U32 status = entry.setCurrentURL(URL_OK);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_OK);
        status = entry.setCurrentURL(URL_TOO_BIG);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_BOUNDS_ERROR);
    }

    TUT_CASE("LLMediaEntry::object_test_7")
    {
        using namespace tut;
        remember_test_name("Test Limits on setting home URL");
        const char *URL_OK = k_url_ok;
        const char *URL_TOO_BIG = k_url_too_big;
        LLMediaEntry entry;
        U32 status = entry.setHomeURL(URL_OK);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_OK);
        status = entry.setHomeURL(URL_TOO_BIG);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_BOUNDS_ERROR);
    }

    TUT_CASE("LLMediaEntry::object_test_8")
    {
        using namespace tut;
        remember_test_name("Test Limits on setting whitelist");
        const char *URL_OK = k_url_ok;
        LLMediaEntry entry;
        std::vector<std::string> whitelist;
        whitelist.push_back(std::string(URL_OK));
        S32 status = entry.setWhiteList(whitelist);
        ensure(get_test_name() + " invalid result", status == LSL_STATUS_OK);
        ensure(get_test_name() + " failed", whitelist == entry.getWhiteList());
    }

    TUT_CASE("LLMediaEntry::object_test_9")
    {
        using namespace tut;
        remember_test_name("Test Limits on setting whitelist too big");
        const char *URL_OK = k_url_ok;
        const char *URL_TOO_BIG = k_url_too_big;
        LLMediaEntry entry;
        std::vector<std::string> whitelist, empty;
        whitelist.push_back(std::string(URL_OK));
        whitelist.push_back(std::string(URL_TOO_BIG));
        S32 status = entry.setWhiteList(whitelist);
        ensure(get_test_name() + " invalid result", status == LSL_STATUS_BOUNDS_ERROR);
        ensure(get_test_name() + " failed", empty == entry.getWhiteList());
    }

    TUT_CASE("LLMediaEntry::object_test_10")
    {
        using namespace tut;
        remember_test_name("Test Limits on setting whitelist too many");
        LLMediaEntry entry;
        std::vector<std::string> whitelist, empty;
        for (int i=0; i < LLMediaEntry::MAX_WHITELIST_SIZE+1; i++)
        {
            whitelist.push_back("Q");
        }
        S32 status = entry.setWhiteList(whitelist);
        ensure(get_test_name() + " invalid result", status == LSL_STATUS_BOUNDS_ERROR);
        ensure(get_test_name() + " failed", empty == entry.getWhiteList());
    }

    TUT_CASE("LLMediaEntry::object_test_11")
    {
        using namespace tut;
        remember_test_name("Test to make sure both setWhiteList() functions behave the same");
        const char *URL_OK = k_url_ok;
        std::vector<std::string> whitelist, empty;
        LLSD whitelist_llsd;
        whitelist.push_back(std::string(URL_OK));
        whitelist_llsd.append(std::string(URL_OK));
        LLMediaEntry entry1, entry2;
        ensure(get_test_name() + " setWhiteList(s) don't match",
               entry1.setWhiteList(whitelist) == LSL_STATUS_OK &&
               entry2.setWhiteList(whitelist_llsd) == LSL_STATUS_OK);
        ensure(get_test_name() + " failed",
               entry1.getWhiteList() == entry2.getWhiteList());
    }

    TUT_CASE("LLMediaEntry::object_test_12")
    {
        using namespace tut;
        remember_test_name("Test to make sure both setWhiteList() functions behave the same");
        const char *URL_OK = k_url_ok;
        const char *URL_TOO_BIG = k_url_too_big;
        std::vector<std::string> whitelist, empty;
        LLSD whitelist_llsd;
        whitelist.push_back(std::string(URL_OK));
        whitelist.push_back(std::string(URL_TOO_BIG));
        whitelist_llsd.append(std::string(URL_OK));
        whitelist_llsd.append(std::string(URL_TOO_BIG));
        LLMediaEntry entry1, entry2;
        ensure(get_test_name() + " setWhiteList(s) don't match",
               entry1.setWhiteList(whitelist) == LSL_STATUS_BOUNDS_ERROR &&
               entry2.setWhiteList(whitelist_llsd) == LSL_STATUS_BOUNDS_ERROR);
        ensure(get_test_name() + " failed",
               empty == entry1.getWhiteList() &&
               empty == entry2.getWhiteList());
    }

    TUT_CASE("LLMediaEntry::object_test_13")
    {
        using namespace tut;
        remember_test_name("Test to make sure both setWhiteList() functions behave the same");
        std::vector<std::string> whitelist, empty;
        LLSD whitelist_llsd;
        for (int i=0; i < LLMediaEntry::MAX_WHITELIST_SIZE+1; i++)
        {
            whitelist.push_back("Q");
            whitelist_llsd.append("Q");
        }
        LLMediaEntry entry1, entry2;
        ensure(get_test_name() + " invalid result",
               entry1.setWhiteList(whitelist) == LSL_STATUS_BOUNDS_ERROR &&
               entry2.setWhiteList(whitelist_llsd) == LSL_STATUS_BOUNDS_ERROR);
        ensure(get_test_name() + " failed",
               empty == entry1.getWhiteList() &&
               empty == entry2.getWhiteList());
    }

    TUT_CASE("LLMediaEntry::object_test_14")
    {
        using namespace tut;
        int n=0;

        whitelist_test(++n, "", "http://www.example.com", true);
        whitelist_test(++n, "www.example.com", "http://www.example.com", true);
        whitelist_test(++n, "http://example.com", "http://example.com", true);
        whitelist_test(++n, false, "www.example.com", "http://www.secondlife.com", true);
        whitelist_test(++n, true, "www.example.com", "http://www.secondlife.com", false);
        whitelist_test(++n, "http://www.example.com", "http://www.example.com/", true);
        whitelist_test(++n, "http://www.example.com/", "http://www.example.com/", true);
        whitelist_test(++n, "http://www.example.com/", "http://www.example.com", false);
        whitelist_test(++n, "http://www.example.com", "http://www.example.com/foobar", true);
        whitelist_test(++n, "http://www.example.com/", "http://www.example.com/foobar", false);
        whitelist_test(++n, "http://example.com", "http://example.com/wiki", true);
        whitelist_test(++n, "www.example.com", "http://www.example.com/help", true);
        whitelist_test(++n, "http://www.example.com", "http://wwwexample.com", false);
        whitelist_test(++n, "http://www.example.com", "http://www.example.com/wiki", true);
        whitelist_test(++n, "example.com", "http://wwwexample.com", false);
        whitelist_test(++n, "http://www.example.com/", "http://www.amazon.com/wiki", false);
        whitelist_test(++n, "www.example.com", "http://www.amazon.com", false);
        whitelist_test(++n, "*.example.com", "http://www.example.com", true);
        whitelist_test(++n, "*.example.com", "http://www.amazon.com", false);
        whitelist_test(++n, "*.example.com", "http://www.example.com/foo/bar", true);
        whitelist_test(++n, "*.example.com", "http:/example.com/foo/bar", false);
        whitelist_test(++n, "*example.com", "http://example.com/foo/bar", true);
        whitelist_test(++n, "*example.com", "http://my.virus.com/foo/bar?example.com", false);
        whitelist_test(++n, "example.com", "http://my.virus.com/foo/bar?example.com", false);
        whitelist_test(++n, "*example.com", "http://my.virus.com/foo/bar?*example.com", false);
        whitelist_test(++n, "http://*example.com", "http://www.example.com", true);
        whitelist_test(++n, "http://*.example.com", "http://www.example.com", true);
        whitelist_test(++n, "http://*.e$?^.com", "http://www.e$?^.com", true);
        whitelist_test(++n, "*.example.com/foo/bar", "http://www.example.com/", false);
        whitelist_test(++n, "*.example.com/foo/bar", "http://example.com/foo/bar", false);
        whitelist_test(++n, "http://*.example.com/foo/bar", "http://www.example.com", false);
        whitelist_test(++n, "http://*.example.com", "https://www.example.com", false);
        whitelist_test(++n, "http*://*.example.com", "rtsp://www.example.com", false);
        whitelist_test(++n, "http*://*.example.com", "https://www.example.com", true);
        whitelist_test(++n, "example.com", "http://www.example.com", false);
        whitelist_test(++n, "www.example.com", "http://www.example.com:80", false);
        whitelist_test(++n, "www.example.com", "http://www.example.com", true);
        whitelist_test(++n, "www.example.com/", "http://www.example.com", false);
        whitelist_test(++n, "www.example.com/foo/bar/*", "http://www.example.com/foo/bar/baz", true);
        whitelist_test(++n, "/foo/*/baz", "http://www.example.com/foo/bar/baz", true);
        whitelist_test(++n, "/foo/*/baz", "http://www.example.com/foo/bar/", false);
    }
}
