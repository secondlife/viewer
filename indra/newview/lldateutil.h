/** 
* @file lldateutil.h
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LLDATEUTIL_H
#define LLDATEUTIL_H

class LLDate;

namespace LLDateUtil
{
	// Convert a date provided by the server (MM/DD/YYYY) into a localized,
	// human-readable age (1 year, 2 months) using translation strings.
	// Pass LLDate::now() for now.
	// Used for avatar inspectors and profiles.
	std::string ageFromDate(const std::string& date_string, const LLDate& now);

	// Calls the above with LLDate::now()
	std::string ageFromDate(const std::string& date_string);

	// As above, for YYYY-MM-DD dates
	std::string ageFromDateISO(const std::string& date_string, const LLDate& now);

	// Calls the above with LLDate::now()
	std::string ageFromDateISO(const std::string& date_string);

	std::string ageFromDate(S32 year, S32 month, S32 day);
}

#endif
