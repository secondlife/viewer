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

#include "doctest.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include "../lldateutil.h"

#include "lldate.h"
#include "llstring.h"
#include "lltrans.h"
#include "llui.h"

#include <map>

std::map<std::string, std::string, std::less<>> gString;
typedef std::pair<std::string, int> count_string_t;
std::map<count_string_t, std::string> gCountString;

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
    return gCountString[count_string_t(xml_desc, count)];
}

std::string LLUI::getLanguage()
{
    return "en";
}

namespace tut
{
    using tut_compat::ensure_equals;
    using tut_compat::set_test_name;

    struct dateutil
    {
        dateutil()
            : mNow(std::string("2009-12-31T08:00:00Z"))
        {
            gString["YearsMonthsOld"] = "[AGEYEARS] [AGEMONTHS] old";
            gString["YearsOld"] = "[AGEYEARS] old";
            gString["MonthsOld"] = "[AGEMONTHS] old";
            gString["WeeksOld"] = "[AGEWEEKS] old";
            gString["DaysOld"] = "[AGEDAYS] old";
            gString["TodayOld"] = "Joined today";

            gCountString[count_string_t("AgeYears", 1)] = "1 year";
            gCountString[count_string_t("AgeYears", 2)] = "2 years";
            gCountString[count_string_t("AgeMonths", 1)] = "1 month";
            gCountString[count_string_t("AgeMonths", 2)] = "2 months";
            gCountString[count_string_t("AgeMonths", 11)] = "11 months";
            gCountString[count_string_t("AgeWeeks", 1)] = "1 week";
            gCountString[count_string_t("AgeWeeks", 2)] = "2 weeks";
            gCountString[count_string_t("AgeWeeks", 3)] = "3 weeks";
            gCountString[count_string_t("AgeWeeks", 4)] = "4 weeks";
            gCountString[count_string_t("AgeDays", 1)] = "1 day";
            gCountString[count_string_t("AgeDays", 2)] = "2 days";
        }
        LLDate mNow;
    };
}

TUT_SUITE("LLDateUtil")
{
    TUT_CASE("LLDateUtil::dateutil_object_t_test_1")
    {
        using namespace tut;
        dateutil data;
        set_test_name("Years");
        ensure_equals("years + months", LLDateUtil::ageFromDate("10/30/2007", data.mNow), "2 years 2 months old");
        ensure_equals("years", LLDateUtil::ageFromDate("12/31/2007", data.mNow), "2 years old");
        ensure_equals("years", LLDateUtil::ageFromDate("1/1/2008", data.mNow), "1 year 11 months old");
        ensure_equals("single year + one month", LLDateUtil::ageFromDate("11/30/2008", data.mNow), "1 year 1 month old");
        ensure_equals("single year + a bit", LLDateUtil::ageFromDate("12/12/2008", data.mNow), "1 year old");
        ensure_equals("single year", LLDateUtil::ageFromDate("12/31/2008", data.mNow), "1 year old");
    }

    TUT_CASE("LLDateUtil::dateutil_object_t_test_2")
    {
        using namespace tut;
        dateutil data;
        set_test_name("Months");
        ensure_equals("months", LLDateUtil::ageFromDate("10/30/2009", data.mNow), "2 months old");
        ensure_equals("months 2", LLDateUtil::ageFromDate("10/31/2009", data.mNow), "2 months old");
        ensure_equals("single month", LLDateUtil::ageFromDate("11/30/2009", data.mNow), "1 month old");
    }

    TUT_CASE("LLDateUtil::dateutil_object_t_test_3")
    {
        using namespace tut;
        dateutil data;
        set_test_name("Weeks");
        ensure_equals("4 weeks", LLDateUtil::ageFromDate("12/1/2009", data.mNow), "4 weeks old");
        ensure_equals("weeks", LLDateUtil::ageFromDate("12/17/2009", data.mNow), "2 weeks old");
        ensure_equals("single week", LLDateUtil::ageFromDate("12/24/2009", data.mNow), "1 week old");
    }

    TUT_CASE("LLDateUtil::dateutil_object_t_test_4")
    {
        using namespace tut;
        dateutil data;
        set_test_name("Days");
        ensure_equals("days", LLDateUtil::ageFromDate("12/29/2009", data.mNow), "2 days old");
        ensure_equals("single day", LLDateUtil::ageFromDate("12/30/2009", data.mNow), "1 day old");
        ensure_equals("today", LLDateUtil::ageFromDate("12/31/2009", data.mNow), "Joined today");
    }

    TUT_CASE("LLDateUtil::dateutil_object_t_test_5")
    {
        using namespace tut;
        set_test_name("2010 rollover");
        LLDate now(std::string("2010-01-04T12:00:00Z"));
        ensure_equals("days", LLDateUtil::ageFromDate("12/13/2009", now), "3 weeks old");
    }
}
