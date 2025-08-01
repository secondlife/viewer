/**
 * @file lldateutil_test.cpp
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

#include "../lldateutil.h"

#include "lldate.h"
#include "llstring.h"   // LLStringUtil::format()
#include "lltrans.h"
#include "llui.h"

#include <map>


// Baked-in return values for getString()
std::map< std::string, std::string, std::less<>> gString;

// Baked-in return values for getCountString()
// map of pairs of input xml_desc and integer count
typedef std::pair< std::string, int > count_string_t;
std::map< count_string_t, std::string > gCountString;

std::string LLTrans::getString(const std::string_view xml_desc, const LLStringUtil::format_map_t& args, bool def_string)
{
    auto it = gString.find(xml_desc);
    if (it != gString.end())
    {
        std::string text = it->second;
        LLStringUtil::format(text, args);
        return text;
    }
    return {};
}

std::string LLTrans::getCountString(std::string_view language, std::string_view xml_desc, S32 count)
{
    count_string_t key(xml_desc, count);
    if (gCountString.find(key) == gCountString.end())
    {
        return std::string("Couldn't find ") + static_cast<std::string>(xml_desc);
    }
    return gCountString[ count_string_t(xml_desc, count) ];
}

std::string LLUI::getLanguage()
{
    return "en";
}

TEST_SUITE("LLDateUtil") {

struct dateutil
{

        // Hard-code a "now" date so unit test doesn't change with
        // current time.  Because server strings are in Pacific time
        // roll this forward 8 hours to compensate.  This represents
        // 2009-12-31T00:00:00Z UTC.
        dateutil()
            :   mNow(std::string("2009-12-31T08:00:00Z"))
        {
            // copied from strings.xml
            gString["YearsMonthsOld"] = "[AGEYEARS] [AGEMONTHS] old";
            gString["YearsOld"] = "[AGEYEARS] old";
            gString["MonthsOld"] = "[AGEMONTHS] old";
            gString["WeeksOld"] = "[AGEWEEKS] old";
            gString["DaysOld"] = "[AGEDAYS] old";
            gString["TodayOld"] = "Joined today";

            gCountString[ count_string_t("AgeYears", 1) ]  = "1 year";
            gCountString[ count_string_t("AgeYears", 2) ]  = "2 years";
            gCountString[ count_string_t("AgeMonths", 1) ] = "1 month";
            gCountString[ count_string_t("AgeMonths", 2) ] = "2 months";
            gCountString[ count_string_t("AgeMonths", 11) ]= "11 months";
            gCountString[ count_string_t("AgeWeeks", 1) ]  = "1 week";
            gCountString[ count_string_t("AgeWeeks", 2) ]  = "2 weeks";
            gCountString[ count_string_t("AgeWeeks", 3) ]  = "3 weeks";
            gCountString[ count_string_t("AgeWeeks", 4) ]  = "4 weeks";
            gCountString[ count_string_t("AgeDays", 1) ]   = "1 day";
            gCountString[ count_string_t("AgeDays", 2) ]   = "2 days";
        
};

TEST_CASE_FIXTURE(dateutil, "test_1")
{

        set_test_name("Years");
        CHECK_MESSAGE(LLDateUtil::ageFromDate("10/30/2007" == mNow, "years + months"),
            "2 years 2 months old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/31/2007" == mNow, "years"),
            "2 years old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("1/1/2008" == mNow, "years"),
            "1 year 11 months old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("11/30/2008" == mNow, "single year + one month"),
            "1 year 1 month old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/12/2008" == mNow, "single year + a bit"),
            "1 year old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/31/2008" == mNow, "single year"),
            "1 year old" );
    
}

TEST_CASE_FIXTURE(dateutil, "test_2")
{

        set_test_name("Months");
        CHECK_MESSAGE(LLDateUtil::ageFromDate("10/30/2009" == mNow, "months"),
            "2 months old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("10/31/2009" == mNow, "months 2"),
            "2 months old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("11/30/2009" == mNow, "single month"),
            "1 month old" );
    
}

TEST_CASE_FIXTURE(dateutil, "test_3")
{

        set_test_name("Weeks");
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/1/2009" == mNow, "4 weeks"),
            "4 weeks old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/17/2009" == mNow, "weeks"),
            "2 weeks old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/24/2009" == mNow, "single week"),
            "1 week old" );
    
}

TEST_CASE_FIXTURE(dateutil, "test_4")
{

        set_test_name("Days");
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/29/2009" == mNow, "days"),
            "2 days old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/30/2009" == mNow, "single day"),
            "1 day old" );
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/31/2009" == mNow, "today"),
            "Joined today" );
    
}

TEST_CASE_FIXTURE(dateutil, "test_5")
{

        set_test_name("2010 rollover");
        LLDate now(std::string("2010-01-04T12:00:00Z"));
        CHECK_MESSAGE(LLDateUtil::ageFromDate("12/13/2009" == now, "days"),
            "3 weeks old" );
    
}

} // TEST_SUITE

