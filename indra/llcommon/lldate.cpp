/** 
 * @file lldate.cpp
 * @author Phoenix
 * @date 2006-02-05
 * @brief Implementation of the date class
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lldate.h"

#include "apr-1/apr_time.h"

#include <iomanip>
#include <sstream>

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
    std::ostringstream stream;
    toHTTPDateStream(stream);
    return stream.str();
}

void LLDate::toHTTPDateStream(std::ostream& s) const
{
    // http://apr.apache.org/docs/apr/0.9/group__apr__time.html
    apr_time_t time = (apr_time_t)(mSecondsSinceEpoch * LL_APR_USEC_PER_SEC);

    apr_time_exp_t exp_time ; //Apache time module

    if (apr_time_exp_gmt(&exp_time, time) != APR_SUCCESS)
    {
        // Return Epoch UTC date
        s << "Thursday, 01 Jan 1970 00:00:00 GMT" ;
        return;
    }

    s << std::dec << std::setfill('0');
#if( LL_WINDOWS || __GNUC__ > 2)
    s << std::right ;
#else
    s.setf(ios::right);
#endif    
    std::string day = weekdays[exp_time.tm_wday];
    std::string month = months[exp_time.tm_mon];

    s << std::setw(day.length()) << (day)
      << ", " << std::setw(2) << (exp_time.tm_mday)
      << ' ' << std::setw(month.length()) << (month)
      << ' ' << std::setw(4) << (exp_time.tm_year + 1900)
	  << ' ' << std::setw(2) << (exp_time.tm_hour)
	  << ':' << std::setw(2) << (exp_time.tm_min)
	  << ':' << std::setw(2) << (exp_time.tm_sec)
      << " GMT";

    // RFC 1123 date does not use microseconds
    llinfos << "Date in RFC 1123 format is " << s << llendl;
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
	s.get(); // skip the Z
	if (c != 'Z') { return false; }

	mSecondsSinceEpoch = seconds_since_epoch;
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
