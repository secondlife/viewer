/**
 * @file lldate.cpp
 * @author Phoenix
 * @date 2006-02-05
 * @brief Implementation of the date class
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
#include "lldate.h"

#include "apr_time.h"

#include <time.h>
#include <locale.h>
#include <string>
#include <iomanip>
#include <sstream>

#include "lltimer.h"
#include "llstring.h"
#include "llfasttimer.h"

static const F64 DATE_EPOCH = 0.0;

static const F64 LL_APR_USEC_PER_SEC = 1000000.0;
    // should be APR_USEC_PER_SEC, but that relies on INT64_C which
    // isn't defined in glib under our build set up for some reason


LLDate::LLDate() : mSecondsSinceEpoch(DATE_EPOCH)
{}

LLDate::LLDate(const LLDate& date) :
    mSecondsSinceEpoch(date.mSecondsSinceEpoch)
{}

LLDate::LLDate(F64SecondsImplicit seconds_since_epoch) :
    mSecondsSinceEpoch(seconds_since_epoch.value())
{}

LLDate::LLDate(const std::string& iso8601_date)
{
    if(!fromString(iso8601_date))
    {
        LL_WARNS() << "date " << iso8601_date << " failed to parse; "
            << "ZEROING IT OUT" << LL_ENDL;
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

std::string LLDate::toHTTPDateString (std::string fmt) const
{
    LL_PROFILE_ZONE_SCOPED;

    time_t locSeconds = (time_t) mSecondsSinceEpoch;
    struct tm * gmt = gmtime (&locSeconds);
    return toHTTPDateString(gmt, fmt);
}

std::string LLDate::toHTTPDateString (tm * gmt, std::string fmt)
{
    LL_PROFILE_ZONE_SCOPED;

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
    std::string res(buffer);
#if LL_WINDOWS
    // Convert from locale-dependant charset to UTF-8 (EXT-8524).
    res = ll_convert_string_to_utf8_string(res);
#endif
    return res;
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
    s        << std::setw(4) << (exp_time.tm_year + 1900)
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
    s << 'Z'
      << std::setfill(' ');
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

    c = s.peek(); // check for offset
    if (c == '+' || c == '-')
    {
        S32 offset_sign = (c == '+') ? 1 : -1;
        S32 offset_hours = 0;
        S32 offset_minutes = 0;
        S32 offset_in_seconds = 0;

        s >> offset_hours;

        c = s.get(); // skip the colon a get the minutes if there are any
        if (c == ':')
        {
            s >> offset_minutes;
        }

        offset_in_seconds =  (offset_hours * 60 + offset_sign * offset_minutes) * 60;
        seconds_since_epoch -= offset_in_seconds;
    }
    else if (c != 'Z') { return false; } // skip the Z

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

