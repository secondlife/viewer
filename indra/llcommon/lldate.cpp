/** 
 * @file lldate.cpp
 * @author Phoenix
 * @date 2006-02-05
 * @brief Implementation of the date class
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "lldate.h"

#include "apr_time.h"

#include <time.h>
#include <locale.h>
#include <string>
#include <iomanip>
#include <sstream>

#include "lltimer.h"
#include "llstring.h"

static const F64 DATE_EPOCH = 0.0;

static const F64 LL_APR_USEC_PER_SEC = 1000000.0;
	// should be APR_USEC_PER_SEC, but that relies on INT64_C which
	// isn't defined in glib under our build set up for some reason


LLDate::LLDate() : mSecondsSinceEpoch(DATE_EPOCH)
{
}

LLDate::LLDate(const LLDate& date) :
	mSecondsSinceEpoch(date.mSecondsSinceEpoch)
{
}

LLDate::LLDate(F64 seconds_since_epoch) :
	mSecondsSinceEpoch(seconds_since_epoch)
{
}

LLDate::LLDate(const std::string& iso8601_date)
{
	if(!fromString(iso8601_date))
	{
		llwarns << "date " << iso8601_date << " failed to parse; "
			<< "ZEROING IT OUT" << llendl;
		mSecondsSinceEpoch = DATE_EPOCH;
	}
}

std::string LLDate::asString() const
{
	std::ostringstream stream;
	toStream(stream);
	return stream.str();
}

//@ brief Converts time in seconds since EPOCH
//        to RFC 1123 compliant date format
//        E.g. 1184797044.037586 == Wednesday, 18 Jul 2007 22:17:24 GMT
//        in RFC 1123. HTTP dates are always in GMT and RFC 1123
//        is one of the standards used and the prefered format
std::string LLDate::asRFC1123() const
{
	return toHTTPDateString (std::string ("%A, %d %b %Y %H:%M:%S GMT"));
}

LLFastTimerUtil::DeclareTimer FT_DATE_FORMAT("Date Format");

std::string LLDate::toHTTPDateString (std::string fmt) const
{
	LLFastTimer ft1(FT_DATE_FORMAT);
	
	time_t locSeconds = (time_t) mSecondsSinceEpoch;
	struct tm * gmt = gmtime (&locSeconds);
	return toHTTPDateString(gmt, fmt);
}

std::string LLDate::toHTTPDateString (tm * gmt, std::string fmt)
{
	LLFastTimer ft1(FT_DATE_FORMAT);

	// avoid calling setlocale() unnecessarily - it's expensive.
	static std::string prev_locale = "";
	std::string this_locale = LLStringUtil::getLocale();
	if (this_locale != prev_locale)
	{
		setlocale(LC_TIME, this_locale.c_str());
		prev_locale = this_locale;
	}

	// use strftime() as it appears to be faster than std::time_put
	char buffer[128];
	strftime(buffer, 128, fmt.c_str(), gmt);
	return std::string(buffer);
}

void LLDate::toStream(std::ostream& s) const
{
	apr_time_t time = (apr_time_t)(mSecondsSinceEpoch * LL_APR_USEC_PER_SEC);
	
	apr_time_exp_t exp_time;
	if (apr_time_exp_gmt(&exp_time, time) != APR_SUCCESS)
	{
		s << "1970-01-01T00:00:00Z";
		return;
	}
	
	s << std::dec << std::setfill('0');
#if( LL_WINDOWS || __GNUC__ > 2)
	s << std::right;
#else
	s.setf(ios::right);
#endif
	s		 << std::setw(4) << (exp_time.tm_year + 1900)
	  << '-' << std::setw(2) << (exp_time.tm_mon + 1)
	  << '-' << std::setw(2) << (exp_time.tm_mday)
	  << 'T' << std::setw(2) << (exp_time.tm_hour)
	  << ':' << std::setw(2) << (exp_time.tm_min)
	  << ':' << std::setw(2) << (exp_time.tm_sec);
	if (exp_time.tm_usec > 0)
	{
		s << '.' << std::setw(2)
		  << (int)(exp_time.tm_usec / (LL_APR_USEC_PER_SEC / 100));
	}
	s << 'Z';
}

bool LLDate::split(S32 *year, S32 *month, S32 *day, S32 *hour, S32 *min, S32 *sec) const
{
	apr_time_t time = (apr_time_t)(mSecondsSinceEpoch * LL_APR_USEC_PER_SEC);
	
	apr_time_exp_t exp_time;
	if (apr_time_exp_gmt(&exp_time, time) != APR_SUCCESS)
	{
		return false;
	}

	if (year)
		*year = exp_time.tm_year + 1900;

	if (month)
		*month = exp_time.tm_mon + 1;

	if (day)
		*day = exp_time.tm_mday;

	if (hour)
		*hour = exp_time.tm_hour;

	if (min)
		*min = exp_time.tm_min;

	if (sec)
		*sec = exp_time.tm_sec;

	return true;
}

bool LLDate::fromString(const std::string& iso8601_date)
{
	std::istringstream stream(iso8601_date);
	return fromStream(stream);
}

bool LLDate::fromStream(std::istream& s)
{
	struct apr_time_exp_t exp_time;
	apr_int32_t tm_part;
	int c;
	
	s >> tm_part;
	exp_time.tm_year = tm_part - 1900;
	c = s.get(); // skip the hypen
	if (c != '-') { return false; }
	s >> tm_part;
	exp_time.tm_mon = tm_part - 1;
	c = s.get(); // skip the hypen
	if (c != '-') { return false; }
	s >> tm_part;
	exp_time.tm_mday = tm_part;
	
	c = s.get(); // skip the T
	if (c != 'T') { return false; }
	
	s >> tm_part;
	exp_time.tm_hour = tm_part;
	c = s.get(); // skip the :
	if (c != ':') { return false; }
	s >> tm_part;
	exp_time.tm_min = tm_part;
	c = s.get(); // skip the :
	if (c != ':') { return false; }
	s >> tm_part;
	exp_time.tm_sec = tm_part;

	// zero out the unused fields
	exp_time.tm_usec = 0;
	exp_time.tm_wday = 0;
	exp_time.tm_yday = 0;
	exp_time.tm_isdst = 0;
	exp_time.tm_gmtoff = 0;

	// generate a time_t from that
	apr_time_t time;
	if (apr_time_exp_gmt_get(&time, &exp_time) != APR_SUCCESS)
	{
		return false;
	}
	
	F64 seconds_since_epoch = time / LL_APR_USEC_PER_SEC;

	// check for fractional
	c = s.peek();
	if(c == '.')
	{
		F64 fractional = 0.0;
		s >> fractional;
		seconds_since_epoch += fractional;
	}
	c = s.get(); // skip the Z
	if (c != 'Z') { return false; }

	mSecondsSinceEpoch = seconds_since_epoch;
	return true;
}

bool LLDate::fromYMDHMS(S32 year, S32 month, S32 day, S32 hour, S32 min, S32 sec)
{
	struct apr_time_exp_t exp_time;
	
	exp_time.tm_year = year - 1900;
	exp_time.tm_mon = month - 1;
	exp_time.tm_mday = day;
	exp_time.tm_hour = hour;
	exp_time.tm_min = min;
	exp_time.tm_sec = sec;

	// zero out the unused fields
	exp_time.tm_usec = 0;
	exp_time.tm_wday = 0;
	exp_time.tm_yday = 0;
	exp_time.tm_isdst = 0;
	exp_time.tm_gmtoff = 0;

	// generate a time_t from that
	apr_time_t time;
	if (apr_time_exp_gmt_get(&time, &exp_time) != APR_SUCCESS)
	{
		return false;
	}
	
	mSecondsSinceEpoch = time / LL_APR_USEC_PER_SEC;

	return true;
}

F64 LLDate::secondsSinceEpoch() const
{
	return mSecondsSinceEpoch;
}

void LLDate::secondsSinceEpoch(F64 seconds)
{
	mSecondsSinceEpoch = seconds;
}

/* static */ LLDate LLDate::now()
{
	// time() returns seconds, we want fractions of a second, which LLTimer provides --RN
	return LLDate(LLTimer::getTotalSeconds());
}

bool LLDate::operator<(const LLDate& rhs) const
{
    return mSecondsSinceEpoch < rhs.mSecondsSinceEpoch;
}

std::ostream& operator<<(std::ostream& s, const LLDate& date)
{
	date.toStream(s);
	return s;
}

std::istream& operator>>(std::istream& s, LLDate& date)
{
	date.fromStream(s);
	return s;
}

