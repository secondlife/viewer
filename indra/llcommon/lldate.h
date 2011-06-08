/** 
 * @file lldate.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of a simple date class.
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

#ifndef LL_LLDATE_H
#define LL_LLDATE_H

#include <iosfwd>
#include <string>

#include "stdtypes.h"

/** 
 * @class LLDate
 * @brief This class represents a particular point in time in UTC.
 *
 * The date class represents a point in time after epoch - 1970-01-01.
 */
class LL_COMMON_API LLDate
{
public:
	/** 
	 * @brief Construct a date equal to epoch.
	 */
	LLDate();

	/** 
	 * @brief Construct a date equal to the source date.
	 */
	LLDate(const LLDate& date);

	/** 
	 * @brief Construct a date from a seconds since epoch value.
	 *
	 * @pararm seconds_since_epoch The number of seconds since UTC epoch.
	 */
	LLDate(F64 seconds_since_epoch);

	/** 
	 * @brief Construct a date from a string representation
	 *
	 * The date is constructed in the <code>fromString()</code>
	 * method. See that method for details of supported formats.
	 * If that method fails to parse the date, the date is set to epoch.
	 * @param iso8601_date An iso-8601 compatible representation of the date.
	 */
	LLDate(const std::string& iso8601_date);

	/** 
	 * @brief Return the date as in ISO-8601 string.
	 *
	 * @return A string representation of the date.
	 */
	std::string asString() const;
	std::string asRFC1123() const;
	void toStream(std::ostream&) const;
	bool split(S32 *year, S32 *month = NULL, S32 *day = NULL, S32 *hour = NULL, S32 *min = NULL, S32 *sec = NULL) const;
	std::string toHTTPDateString (std::string fmt) const;
	static std::string toHTTPDateString (tm * gmt, std::string fmt);
	/** 
	 * @brief Set the date from an ISO-8601 string.
	 *
	 * The parser only supports strings conforming to
	 * YYYYF-MM-DDTHH:MM:SS.FFZ where Y is year, M is month, D is day,
	 * H is hour, M is minute, S is second, F is sub-second, and all
	 * other characters are literal.
	 * If this method fails to parse the date, the previous date is
	 * retained.
	 * @param iso8601_date An iso-8601 compatible representation of the date.
	 * @return Returns true if the string was successfully parsed.
	 */
	bool fromString(const std::string& iso8601_date);
	bool fromStream(std::istream&);
	bool fromYMDHMS(S32 year, S32 month = 1, S32 day = 0, S32 hour = 0, S32 min = 0, S32 sec = 0);

	/** 
	 * @brief Return the date in seconds since epoch.
	 *
	 * @return The number of seconds since epoch UTC.
	 */
	F64 secondsSinceEpoch() const;

	/** 
	 * @brief Set the date in seconds since epoch.
	 *
	 * @param seconds The number of seconds since epoch UTC.
	 */
	void secondsSinceEpoch(F64 seconds);
    
    /**
     * @brief Create an LLDate object set to the current time.
	 *
	 * @return The number of seconds since epoch UTC.
	 */
    static LLDate now();

	/** 
	 * @brief Compare dates using operator< so we can order them using STL.
	 *
	 * @param rhs -- the right hand side of the comparison operator
	 */
	bool operator<(const LLDate& rhs) const;
    
	/** 
	 * @brief Remaining comparison operators in terms of operator<
     * This conforms to the expectation of STL.
	 *
	 * @param rhs -- the right hand side of the comparison operator
	 */
    bool operator>(const LLDate& rhs) const { return rhs < *this; }
    bool operator<=(const LLDate& rhs) const { return !(rhs < *this); }
    bool operator>=(const LLDate& rhs) const { return !(*this < rhs); }
    bool operator!=(const LLDate& rhs) const { return (*this < rhs) || (rhs < *this); }
    bool operator==(const LLDate& rhs) const { return !(*this != rhs); }

	/**
	 * @brief Compare to epoch UTC.
	 */

	bool isNull() const { return mSecondsSinceEpoch == 0.0; }
	bool notNull() const { return mSecondsSinceEpoch != 0.0; }
	 

private:
	F64 mSecondsSinceEpoch;
};

// Helper function to stream out a date
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLDate& date);

// Helper function to stream in a date
LL_COMMON_API std::istream& operator>>(std::istream& s, LLDate& date);



#endif // LL_LLDATE_H
