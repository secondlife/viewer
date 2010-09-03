/** 
 * @file lldateutil_test.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "../test/lltut.h"

#include "../lldateutil.h"

#include "lldate.h"
#include "llstring.h"	// LLStringUtil::format()
#include "lltrans.h"
#include "llui.h"

#include <map>


// Baked-in return values for getString()
std::map< std::string, std::string > gString;

// Baked-in return values for getCountString()
// map of pairs of input xml_desc and integer count
typedef std::pair< std::string, int > count_string_t;
std::map< count_string_t, std::string > gCountString;

std::string LLTrans::getString(const std::string &xml_desc, const LLStringUtil::format_map_t& args)
{
	std::string text = gString[xml_desc];
	LLStringUtil::format(text, args);
	return text;
}

std::string LLTrans::getCountString(const std::string& language, const std::string& xml_desc, S32 count)
{
	count_string_t key(xml_desc, count);
	if (gCountString.find(key) == gCountString.end())
	{
		return std::string("Couldn't find ") + xml_desc;
	}
	return gCountString[ count_string_t(xml_desc, count) ];
}

std::string LLUI::getLanguage()
{
	return "en";
}

namespace tut
{
    struct dateutil
    {
		// Hard-code a "now" date so unit test doesn't change with
		// current time.  Because server strings are in Pacific time
		// roll this forward 8 hours to compensate.  This represents
		// 2009-12-31T00:00:00Z UTC.
		dateutil()
			:	mNow(std::string("2009-12-31T08:00:00Z"))
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
		}
		LLDate mNow;
    };
    
	typedef test_group<dateutil> dateutil_t;
	typedef dateutil_t::object dateutil_object_t;
	tut::dateutil_t tut_dateutil("dateutil");

	template<> template<>
	void dateutil_object_t::test<1>()
	{
		set_test_name("Years");
		ensure_equals("years + months",
			LLDateUtil::ageFromDate("10/30/2007", mNow),
			"2 years 2 months old" );
		ensure_equals("years",
			LLDateUtil::ageFromDate("12/31/2007", mNow),
			"2 years old" );
		ensure_equals("years",
			LLDateUtil::ageFromDate("1/1/2008", mNow),
			"1 year 11 months old" );
		ensure_equals("single year + one month",
			LLDateUtil::ageFromDate("11/30/2008", mNow),
			"1 year 1 month old" );
		ensure_equals("single year + a bit",
			LLDateUtil::ageFromDate("12/12/2008", mNow),
			"1 year old" );
		ensure_equals("single year",
			LLDateUtil::ageFromDate("12/31/2008", mNow),
			"1 year old" );
    }

	template<> template<>
	void dateutil_object_t::test<2>()
	{
		set_test_name("Months");
		ensure_equals("months",
			LLDateUtil::ageFromDate("10/30/2009", mNow),
			"2 months old" );
		ensure_equals("months 2",
			LLDateUtil::ageFromDate("10/31/2009", mNow),
			"2 months old" );
		ensure_equals("single month",
			LLDateUtil::ageFromDate("11/30/2009", mNow),
			"1 month old" );
	}

	template<> template<>
	void dateutil_object_t::test<3>()
	{
		set_test_name("Weeks");
		ensure_equals("4 weeks",
			LLDateUtil::ageFromDate("12/1/2009", mNow),
			"4 weeks old" );
		ensure_equals("weeks",
			LLDateUtil::ageFromDate("12/17/2009", mNow),
			"2 weeks old" );
		ensure_equals("single week",
			LLDateUtil::ageFromDate("12/24/2009", mNow),
			"1 week old" );
	}

	template<> template<>
	void dateutil_object_t::test<4>()
	{
		set_test_name("Days");
		ensure_equals("days",
			LLDateUtil::ageFromDate("12/29/2009", mNow),
			"2 days old" );
		ensure_equals("single day",
			LLDateUtil::ageFromDate("12/30/2009", mNow),
			"1 day old" );
		ensure_equals("today",
			LLDateUtil::ageFromDate("12/31/2009", mNow),
			"Joined today" );
	}

	template<> template<>
	void dateutil_object_t::test<5>()
	{
		set_test_name("2010 rollover");
		LLDate now(std::string("2010-01-04T12:00:00Z"));
		ensure_equals("days",
			LLDateUtil::ageFromDate("12/13/2009", now),
			"3 weeks old" );
	}

	//template<> template<>
	//void dateutil_object_t::test<6>()
	//{
	//	set_test_name("ISO dates");
	//	LLDate now(std::string("2010-01-04T12:00:00Z"));
	//	ensure_equals("days",
	//		LLDateUtil::ageFromDateISO("2009-12-13", now),
	//		"3 weeks old" );
	//}
}
