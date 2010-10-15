/** 
* @file lldateutil.h
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

#ifndef LLDATEUTIL_H
#define LLDATEUTIL_H

class LLDate;

namespace LLDateUtil
{
	/**
	 * Convert a date provided by the server into seconds since the Epoch.
	 * 
	 * @param[out] date Number of seconds since 01/01/1970 UTC.
	 * @param[in]  str  Date string (MM/DD/YYYY) in PDT time zone.
	 * 
	 * @return true on success, false on parse error
	 */
	bool dateFromPDTString(LLDate& date, const std::string& str);

	/**
	 * Get human-readable avatar age.
	 * 
	 * Used for avatar inspectors and profiles.
	 * 
	 * @param born_date Date an avatar was born on.
	 * @param now       Current date.
	 * 
	 * @return human-readable localized string like "1 year, 2 months",
	 *         or "???" on error.
	 */
	std::string ageFromDate(const LLDate& born_date, const LLDate& now);

	// Convert a date provided by the server (MM/DD/YYYY) into a localized,
	// human-readable age (1 year, 2 months) using translation strings.
	// Pass LLDate::now() for now.
	// Used for avatar inspectors and profiles.
	std::string ageFromDate(const std::string& date_string, const LLDate& now);

	// Calls the above with LLDate::now()
	std::string ageFromDate(const std::string& date_string);

	// As above, for YYYY-MM-DD dates
	//std::string ageFromDateISO(const std::string& date_string, const LLDate& now);

	// Calls the above with LLDate::now()
	//std::string ageFromDateISO(const std::string& date_string);

	//std::string ageFromDate(S32 born_year, S32 born_month, S32 born_day, const LLDate& now);
}

#endif
