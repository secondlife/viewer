/** 
 * @file lldate.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of a simple date class.
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
	std::string toHTTPDateString (std::string fmt) const;
	static std::string toHTTPDateString (tm * gmt, std::string fmt);
	static void toHTTPDateStream(std::ostream&, tm *, std::string);
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


const static std::string weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const static std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#endif // LL_LLDATE_H
