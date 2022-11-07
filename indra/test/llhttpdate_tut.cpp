/** 
 * @file llhttpdate_tut.cpp
 * @author Kartic Krishnamurthy
 * @date Wednesday, 18 Jul 2007 17:00:00 GMT :)
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

#include "linden_common.h"
#include "lltut.h"

#include "lldate.h"
#include "llcommon.h"
#include "llframetimer.h"

#include <time.h>
#include <locale.h>

namespace tut
{
    struct httpdate_data
    {
        httpdate_data() 
        { 
        }
        ~httpdate_data() 
        { 
        }
        LLDate some_date;
    };
    typedef test_group<httpdate_data> httpdate_test;
    typedef httpdate_test::object httpdate_object;
    tut::httpdate_test httpdate("httpdate");

    template<> template<>
    void httpdate_object::test<1>()
    {

        static std::string epoch_expected = "Thursday, 01 Jan 1970 00:00:00 GMT" ;
        ensure("Check Epoch in RFC 1123", ( epoch_expected == some_date.asRFC1123()));
    }

    template<> template<>
    void httpdate_object::test<2>()
    {

        static std::string expected = "Wednesday, 18 Jul 2007 22:17:24 GMT" ;
        some_date = LLDate(1184797044.037586);
        ensure("Check some timestamp in RFC 1123", ( expected == some_date.asRFC1123()));
    }

    // This test of course most generic.. runs off current time
    template<> template<>
    void httpdate_object::test<3>()
    {

        //F64 sometime = LLFrameTimer::getTotalSeconds();
        time_t sometime;
        time(&sometime);
        some_date = LLDate((F64) sometime);
        struct tm *result;
        char expected[255];
        std::string actual;
        
        result = gmtime(&sometime);
        /*
        std::cout << " seconds: "<< result->tm_sec 
                  << ", minutes: " << result->tm_min
                  << ", hours: " << result->tm_hour
                  << ", day of the month: " << result->tm_mday
                  << ", month: " << result->tm_mon
                  << ", year: " << result->tm_year
                  << ", day of the week: " << result->tm_wday
                  << ", day in the year: " << result->tm_yday
                  << ", DST: " << result->tm_isdst << std::endl;
        */
        strftime(expected, 255, "%A, %d %b %Y %H:%M:%S GMT", result);
        actual = some_date.asRFC1123();
        // probably not a good idea to use strcmp but this is just a unit test
        ensure("Current time in RFC 1123", (strcmp(expected, actual.c_str()) == 0));
    }

    void test_date_string(const std::string &locale, struct tm *t,
                          const std::string &fmt, const std::string &expected)
    {
        std::string result = LLDate::toHTTPDateString(t, fmt);
        LLStringUtil::toLower(result);
        std::string label = std::string("toHTTPDateString - ") + locale;
        ensure_equals(label.c_str(), result, expected);
    }

    template<> template<>
    void httpdate_object::test<4>()
    {
        // test localization of http dates
#if LL_WINDOWS
        const char *en_locale = "english";
        const char *fr_locale = "french";
#else
        const char *en_locale = "en_GB.UTF-8";
        const char *fr_locale = "fr_FR.UTF-8";
#endif

        std::string prev_locale = LLStringUtil::getLocale();
        std::string prev_clocale = std::string(setlocale(LC_TIME, NULL));
        time_t test_time = 1252374030;  // 8 Sep 2009 01:40:01
        struct tm *t = gmtime(&test_time);

        setlocale(LC_TIME, en_locale);
        if (strcmp(setlocale(LC_TIME, NULL), en_locale) != 0)
        {
            setlocale(LC_TIME, prev_clocale.c_str());
            skip("Cannot set English locale");
        }

        LLStringUtil::setLocale(en_locale);
        test_date_string(en_locale, t, "%d %B %Y - %H:%M", "08 september 2009 - 01:40");
        test_date_string(en_locale, t, "%H", "01");
        test_date_string(en_locale, t, "%M", "40");
        test_date_string(en_locale, t, "%I", "01");
        test_date_string(en_locale, t, "%d", "08");
        test_date_string(en_locale, t, "%Y", "2009");
        test_date_string(en_locale, t, "%p", "am");
        test_date_string(en_locale, t, "%A", "tuesday");
        test_date_string(en_locale, t, "%B", "september");

        setlocale(LC_TIME, fr_locale);
        if (strcmp(setlocale(LC_TIME, NULL), fr_locale) != 0)
        {
            LLStringUtil::setLocale(prev_locale);
            setlocale(LC_TIME, prev_clocale.c_str());
            skip("Cannot set French locale");
        }

        LLStringUtil::setLocale(fr_locale);
        test_date_string(fr_locale, t, "%d %B %Y - %H:%M", "08 septembre 2009 - 01:40");
        test_date_string(fr_locale, t, "%H", "01");
        test_date_string(fr_locale, t, "%M", "40");
        test_date_string(fr_locale, t, "%I", "01");
        test_date_string(fr_locale, t, "%d", "08");
        test_date_string(fr_locale, t, "%Y", "2009");
        test_date_string(fr_locale, t, "%A", "mardi");
        test_date_string(fr_locale, t, "%B", "septembre");

        LLStringUtil::setLocale(prev_locale);
        setlocale(LC_TIME, prev_clocale.c_str());
    }
}
