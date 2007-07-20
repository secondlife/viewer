/** 
 * @file llhttpdate_tut.cpp
 * @author Kartic Krishnamurthy
 * @date Wednesday, 18 Jul 2007 17:00:00 GMT :)
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lltut.h"

#include "lldate.h"
#include "llframetimer.h"

#include <string>
#include <time.h>

namespace tut
{
    struct httpdate_data
    {
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
        struct tm result;
        char expected[255], *actual;
        
        gmtime_r((time_t *)&sometime, &result);
        /*
        std::cout << " seconds: "<< result.tm_sec 
                  << ", minutes: " << result.tm_min
                  << ", hours: " << result.tm_hour
                  << ", day of the month: " << result.tm_mday
                  << ", month: " << result.tm_mon
                  << ", year: " << result.tm_year
                  << ", day of the week: " << result.tm_wday
                  << ", day in the year: " << result.tm_yday
                  << ", DST: " << result.tm_isdst << std::endl;
        */
        strftime(expected, 255, "%A, %d %h %Y %H:%M:%S GMT", &result);
        actual = (char *) some_date.asRFC1123().c_str();
        // probably not a good idea to use strcmp but this is just a unit test
        ensure("Current time in RFC 1123", (strcmp(expected, actual) == 0));
    }
}
