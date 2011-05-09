/** 
* @file lldateutil.cpp
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lldateutil.h"

// Linden libraries
#include "lltrans.h"
#include "llui.h"

static S32 DAYS_PER_MONTH_NOLEAP[] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static S32 DAYS_PER_MONTH_LEAP[] =
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static S32 days_from_month(S32 year, S32 month)
{
	llassert_always(1 <= month);
	llassert_always(month <= 12);

	if (year % 4 == 0 
		&& year % 100 != 0)
	{
		// leap year
		return DAYS_PER_MONTH_LEAP[month - 1];
	}
	else
	{
		return DAYS_PER_MONTH_NOLEAP[month - 1];
	}
}

bool LLDateUtil::dateFromPDTString(LLDate& date, const std::string& str)
{
	S32 month, day, year;
	S32 matched = sscanf(str.c_str(), "%d/%d/%d", &month, &day, &year);
	if (matched != 3) return false;
	date.fromYMDHMS(year, month, day);
	F64 secs_since_epoch = date.secondsSinceEpoch();
	// Correct for the fact that specified date is in Pacific time, == UTC - 8
	secs_since_epoch += 8.0 * 60.0 * 60.0;
	date.secondsSinceEpoch(secs_since_epoch);
	return true;
}

std::string LLDateUtil::ageFromDate(const LLDate& born_date, const LLDate& now)
{
	S32 born_month, born_day, born_year;
	// explode out to month/day/year again
	born_date.split(&born_year, &born_month, &born_day);

	S32 now_year, now_month, now_day;
	now.split(&now_year, &now_month, &now_day);

	// Do grade-school subtraction, from right-to-left, borrowing from the left
	// when things go negative
	S32 age_days = (now_day - born_day);
	if (age_days < 0)
	{
		now_month -= 1;
		if (now_month == 0)
		{
			now_year -= 1;
			now_month = 12;
		}
		age_days += days_from_month(now_year, now_month);
	}
	S32 age_months = (now_month - born_month);
	if (age_months < 0)
	{
		now_year -= 1;
		age_months += 12;
	}
	S32 age_years = (now_year - born_year);

	// Noun pluralization depends on language
	std::string lang = LLUI::getLanguage();

	// Try for age in round number of years
	LLStringUtil::format_map_t args;

	if (age_months > 0 || age_years > 0)
	{
		args["[AGEYEARS]"] =
			LLTrans::getCountString(lang, "AgeYears", age_years);
		args["[AGEMONTHS]"] =
			LLTrans::getCountString(lang, "AgeMonths", age_months);

		// We want to display times like:
		// 2 year 2 months
		// 2 years (implicitly 0 months)
		// 11 months
		if (age_years > 0)
		{
			if (age_months > 0)
			{
				return LLTrans::getString("YearsMonthsOld", args);
			}
			else
			{
				return LLTrans::getString("YearsOld", args);
			}
		}
		else // age_years == 0
		{
			return LLTrans::getString("MonthsOld", args);
		}
	}
	// you're 0 months old, display in weeks or days

	// Now for age in weeks
	S32 age_weeks = age_days / 7;
	age_days = age_days % 7;
	if (age_weeks > 0)
	{
		args["[AGEWEEKS]"] = 
			LLTrans::getCountString(lang, "AgeWeeks", age_weeks);
		return LLTrans::getString("WeeksOld", args);
	}

	// Down to days now
	if (age_days > 0)
	{
		args["[AGEDAYS]"] =
			LLTrans::getCountString(lang, "AgeDays", age_days);
		return LLTrans::getString("DaysOld", args);
	}

	return LLTrans::getString("TodayOld");
}

std::string LLDateUtil::ageFromDate(const std::string& date_string, const LLDate& now)
{
	LLDate born_date;

	if (!dateFromPDTString(born_date, date_string))
		return "???";

	return ageFromDate(born_date, now);
}

std::string LLDateUtil::ageFromDate(const std::string& date_string)
{
	return ageFromDate(date_string, LLDate::now());
}

//std::string LLDateUtil::ageFromDateISO(const std::string& date_string,
//									   const LLDate& now)
//{
//	S32 born_month, born_day, born_year;
//	S32 matched = sscanf(date_string.c_str(), "%d-%d-%d",
//			&born_year, &born_month, &born_day);
//	if (matched != 3) return "???";
//	date.fromYMDHMS(year, month, day);
//	F64 secs_since_epoch = date.secondsSinceEpoch();
//	// Correct for the fact that specified date is in Pacific time, == UTC - 8
//	secs_since_epoch += 8.0 * 60.0 * 60.0;
//	date.secondsSinceEpoch(secs_since_epoch);
//	return ageFromDate(born_year, born_month, born_day, now);
//}
//
//std::string LLDateUtil::ageFromDateISO(const std::string& date_string)
//{
//	return ageFromDateISO(date_string, LLDate::now());
//}
