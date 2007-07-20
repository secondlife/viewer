/** 
 * @file lldate.h
 * @author Phoenix
 * @date 2006-02-05
 * @brief Declaration of a simple date class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDATE_H
#define LL_LLDATE_H

#include <iosfwd>

#include "stdtypes.h"

/** 
 * @class LLDate
 * @brief This class represents a particular point in time in UTC.
 *
 * The date class represents a point in time after epoch - 1970-01-01.
 */
class LLDate
{
public:
	/** 
	 * @brief Construct a date equal to epoch.
	 */
	LLDate();

	/** 
	 * @brief Construct a date equal to epoch.
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
	void toHTTPDateStream(std::ostream&) const;
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

private:
	F64 mSecondsSinceEpoch;
};


// Helper function to stream out a date
std::ostream& operator<<(std::ostream& s, const LLDate& date);

// Helper function to stream in a date
std::istream& operator>>(std::istream& s, LLDate& date);


const static std::string weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const static std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#endif // LL_LLDATE_H
