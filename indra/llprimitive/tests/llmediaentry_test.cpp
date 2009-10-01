/** 
 * @file llmediaentry_test.cpp
 * @brief llmediaentry unit tests
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$                               
 * Copyright (c) 2001-2009, Linden Research, Inc.                               
 * $/LicenseInfo$                                                               
 */

#include "linden_common.h"
#include "lltut.h"
#include "boost/lexical_cast.hpp"
#include "llstring.h"
#include "llsdutil.h"
#include "llsdserialize.h"

#include "../llmediaentry.h"
#include "lllslconstants.h"

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

namespace tut
{
    // this is fixture data that gets created before each test and destroyed
    // after each test.  this is where we put all of the setup/takedown code
    // and data needed for each test.
    struct MediaEntry_test
    {
        MediaEntry_test() {
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

    typedef test_group<MediaEntry_test, 55> factory;
    typedef factory::object object;
}
        

namespace
{
    // this is for naming our tests to make pretty output
    tut::factory tf("MediaEntry Test");
}

namespace tut
{
    bool llsd_equals(const LLSD& a, const LLSD& b) {
        // cheesy, brute force, but it works
        return std::string(ll_pretty_print_sd(a)) == std::string(ll_pretty_print_sd(b));
    }

    void ensure_llsd_equals(const std::string& msg, const LLSD& expected, const LLSD& actual)
    {
        if (! llsd_equals(expected, actual))
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
        if ( expected != actual )
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

    void whitelist_test(bool enable, const char *whitelist, const char *candidate_url, bool expected_pass)
    {
        std::string message = "Whitelist test";
        LLMediaEntry entry;
        entry.setWhiteListEnable(enable);
        set_whitelist(entry, whitelist);
        bool passed_whitelist = entry.checkCandidateUrl(candidate_url);
        if (passed_whitelist != expected_pass)
        {
            message += " failed: expected ";
            message += (expected_pass) ? "" : "NOT ";
            message += "to match\nwhitelist = ";
            message += whitelist;
            message += "\ncandidate_url = ";
            message += candidate_url;
        }
        ensure(message, expected_pass == passed_whitelist);
    }

    void whitelist_test(const char *whitelist, const char *candidate_url, bool expected_pass)
    {
        whitelist_test(true, whitelist, candidate_url, expected_pass);
    }
    void whitelist_test(const char *whitelist, const char *candidate_url)
    {
        whitelist_test(true, whitelist, candidate_url, true);
    }

	template<> template<>
	void object::test<1>()
	{
		set_test_name("Test LLMediaEntry Instantiation");
		LLMediaEntry entry;
        ensure_llsd_equals(get_test_name(), defaultMediaEntryLLSD, entry.asLLSD());

	}

	template<> template<>
	void object::test<2>()
	{
		set_test_name("Test LLMediaEntry Instantiation from LLSD");
        LLMediaEntry entry;
        LLSD sd;
        entry.fromLLSD(sd);
        ensure_llsd_equals(get_test_name() + " failed", emptyMediaEntryLLSD, entry.asLLSD());
    }

    template<> template<>
    void object::test<3>()
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

    // limit tests
    const char *URL_OK = "http://www.example.com";
    const char *URL_TOO_BIG = "http://www.example.com.qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";

    template<> template<>
    void object::test<4>()
    {
        set_test_name("Test Limits on setting current URL");
        LLMediaEntry entry;
        U32 status = entry.setCurrentURL(URL_OK);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_OK);
        status = entry.setCurrentURL(URL_TOO_BIG);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_BOUNDS_ERROR);
    }    

    template<> template<>
    void object::test<5>()
    {
        set_test_name("Test Limits on setting home URL");
        LLMediaEntry entry;
        U32 status = entry.setHomeURL(URL_OK);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_OK);
        status = entry.setHomeURL(URL_TOO_BIG);
        ensure(get_test_name() + " ok failed", status == LSL_STATUS_BOUNDS_ERROR);
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("Test Limits on setting whitelist");
        
        // Test a valid list
        LLMediaEntry entry;
        std::vector<std::string> whitelist;
        whitelist.push_back(std::string(URL_OK));
        S32 status = entry.setWhiteList(whitelist);
        ensure(get_test_name() + " invalid result", status == LSL_STATUS_OK);
        ensure(get_test_name() + " failed", whitelist == entry.getWhiteList());
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("Test Limits on setting whitelist too big");
        
        // Test an invalid list
        LLMediaEntry entry;
        std::vector<std::string> whitelist, empty;
        whitelist.push_back(std::string(URL_OK));
        whitelist.push_back(std::string(URL_TOO_BIG));
        S32 status = entry.setWhiteList(whitelist);
        ensure(get_test_name() + " invalid result", status == LSL_STATUS_BOUNDS_ERROR);
        ensure(get_test_name() + " failed", empty == entry.getWhiteList());
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("Test Limits on setting whitelist too many");
        
        // Test an invalid list
        LLMediaEntry entry;
        std::vector<std::string> whitelist, empty;
        for (int i=0; i < LLMediaEntry::MAX_WHITELIST_SIZE+1; i++) {
            whitelist.push_back("Q");
        }
        S32 status = entry.setWhiteList(whitelist);
        ensure(get_test_name() + " invalid result", status == LSL_STATUS_BOUNDS_ERROR);
        ensure(get_test_name() + " failed", empty == entry.getWhiteList());
    }

    template<> template<>
    void object::test<9>()
    {
        set_test_name("Test to make sure both setWhiteList() functions behave the same");
        
        // Test a valid list
        std::vector<std::string> whitelist, empty;
        LLSD whitelist_llsd;
        whitelist.push_back(std::string(URL_OK));
        whitelist_llsd.append(std::string(URL_OK));
        LLMediaEntry entry1, entry2;
        ensure(get_test_name() + " setWhiteList(s) don't match",
               entry1.setWhiteList(whitelist) == LSL_STATUS_OK &&
               entry2.setWhiteList(whitelist_llsd)== LSL_STATUS_OK );
        ensure(get_test_name() + " failed", 
               entry1.getWhiteList() == entry2.getWhiteList());
    }
    
    template<> template<>
    void object::test<10>()
    {
        set_test_name("Test to make sure both setWhiteList() functions behave the same");

        // Test an invalid list
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

    template<> template<>
    void object::test<11>()
    {
        set_test_name("Test to make sure both setWhiteList() functions behave the same");

        // Test an invalid list, too many
        std::vector<std::string> whitelist, empty;
        LLSD whitelist_llsd;
        for (int i=0; i < LLMediaEntry::MAX_WHITELIST_SIZE+1; i++) {
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
    
    // Whitelist check tests
    
    // Check the "empty whitelist" case
    template<> template<>
    void object::test<12>() { whitelist_test("", "http://www.example.com", true); }

    // Check the "missing scheme" case
    template<> template<>
    void object::test<13>() { whitelist_test("www.example.com", "http://www.example.com", true); }

    // Check the "exactly the same" case
    template<> template<>
    void object::test<14>() { whitelist_test("http://example.com", "http://example.com", true); }

    // Check the enable flag
    template<> template<>
    void object::test<15>() { whitelist_test(false, "www.example.com", "http://www.secondlife.com", true); }
    template<> template<>
    void object::test<16>() { whitelist_test(true, "www.example.com", "http://www.secondlife.com", false); }

    // Check permutations of trailing slash:
    template<> template<>
    void object::test<17>() { whitelist_test("http://www.example.com", "http://www.example.com/", true); }
    template<> template<>
    void object::test<18>() { whitelist_test("http://www.example.com/", "http://www.example.com/", true); }
    template<> template<>
    void object::test<19>() { whitelist_test("http://www.example.com/", "http://www.example.com", false); }
    template<> template<>
    void object::test<20>() { whitelist_test("http://www.example.com", "http://www.example.com/foobar", true); }
    template<> template<>
    void object::test<21>() { whitelist_test("http://www.example.com/", "http://www.example.com/foobar", false); }

    
    // More cases...
    template<> template<>
    void object::test<22>() { whitelist_test("http://example.com", "http://example.com/wiki", true); }
    template<> template<>
    void object::test<23>() { whitelist_test("www.example.com", "http://www.example.com/help", true); }
    template<> template<>
    void object::test<24>() { whitelist_test("http://www.example.com", "http://wwwexample.com", false); }
    template<> template<>
    void object::test<25>() { whitelist_test("http://www.example.com", "http://www.example.com/wiki", true); }
    template<> template<>
    void object::test<26>() { whitelist_test("example.com", "http://wwwexample.com", false); }
    template<> template<>
    void object::test<27>() { whitelist_test("http://www.example.com/", "http://www.amazon.com/wiki", false); }
    template<> template<>
    void object::test<28>() { whitelist_test("www.example.com", "http://www.amazon.com", false); }

    // regexp cases
    template<> template<>
    void object::test<29>() { whitelist_test("*.example.com", "http://www.example.com", true); }
    template<> template<>
    void object::test<30>() { whitelist_test("*.example.com", "http://www.amazon.com", false); }
    template<> template<>
    void object::test<31>() { whitelist_test("*.example.com", "http://www.example.com/foo/bar", true); }
    template<> template<>
    void object::test<32>() { whitelist_test("*.example.com", "http:/example.com/foo/bar", false); }
    template<> template<>
    void object::test<33>() { whitelist_test("*example.com", "http://example.com/foo/bar", true); }
    template<> template<>
    void object::test<34>() { whitelist_test("*example.com", "http://my.virus.com/foo/bar?example.com", false); }
    template<> template<>
    void object::test<35>() { whitelist_test("example.com", "http://my.virus.com/foo/bar?example.com", false); }
    template<> template<>
    void object::test<36>() { whitelist_test("*example.com", "http://my.virus.com/foo/bar?*example.com", false); }
    template<> template<>
    void object::test<37>() { whitelist_test("http://*example.com", "http://www.example.com", true); }
    template<> template<>
    void object::test<38>() { whitelist_test("http://*.example.com", "http://www.example.com", true); }
    template<> template<>
    void object::test<39>() { whitelist_test("http://*.e$?^.com", "http://www.e$?^.com", true); }
    template<> template<>
    void object::test<40>() { whitelist_test("*.example.com/foo/bar", "http://www.example.com/", false); }
    template<> template<>
    void object::test<41>() { whitelist_test("*.example.com/foo/bar", "http://example.com/foo/bar", false); }
    template<> template<>
    void object::test<42>() { whitelist_test("http://*.example.com/foo/bar", "http://www.example.com", false); }
    template<> template<>
    void object::test<43>() { whitelist_test("http://*.example.com", "https://www.example.com", false); }
    template<> template<>
    void object::test<44>() { whitelist_test("http*://*.example.com", "rtsp://www.example.com", false); }
    template<> template<>
    void object::test<45>() { whitelist_test("http*://*.example.com", "https://www.example.com", true); }
    template<> template<>
    void object::test<46>() { whitelist_test("example.com", "http://www.example.com", false); }
    template<> template<>
    void object::test<47>() { whitelist_test("www.example.com", "http://www.example.com:80", false); }
    template<> template<>
    void object::test<48>() { whitelist_test("www.example.com", "http://www.example.com", true); }
    template<> template<>
    void object::test<49>() { whitelist_test("www.example.com/", "http://www.example.com", false); }
    template<> template<>
    void object::test<50>() { whitelist_test("www.example.com/foo/bar/*", "http://www.example.com/foo/bar/baz", true); }
    // Path only
    template<> template<>
    void object::test<51>() { whitelist_test("/foo/*/baz", "http://www.example.com/foo/bar/baz", true); }
    template<> template<>
    void object::test<52>() { whitelist_test("/foo/*/baz", "http://www.example.com/foo/bar/", false); }
}
