/** 
 * @file llurlmatch.h
 * @author Martin Reddy
 * @brief Specifies a matched Url in a string, as returned by LLUrlRegistry
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

#ifndef LL_LLURLMATCH_H
#define LL_LLURLMATCH_H

#include "linden_common.h"

#include <string>
#include <vector>
#include "lluicolor.h"

///
/// LLUrlMatch describes a single Url that was matched within a string by 
/// the LLUrlRegistry::findUrl() method. It includes the actual Url that
/// was matched along with its first/last character offset in the string.
/// An alternate label is also provided for creating a hyperlink, as well
/// as tooltip/status text, an icon, and a XUI file for a context menu
/// that can be used in a popup for a Url (e.g., Open, Copy URL, etc.)
///
class LLUrlMatch
{
public:
	LLUrlMatch();

	/// return true if this object does not contain a valid Url match yet
	bool empty() const { return mUrl.empty(); }

	/// return the offset in the string for the first character of the Url
	U32 getStart() const { return mStart; }

	/// return the offset in the string for the last character of the Url
	U32 getEnd() const { return mEnd; }

	/// return the Url that has been matched in the input string
	std::string getUrl() const { return mUrl; }

	/// return a label that can be used for the display of this Url
	std::string getLabel() const { return mLabel; }

	/// return a message that could be displayed in a tooltip or status bar
	std::string getTooltip() const { return mTooltip; }

	/// return the filename for an icon that can be displayed next to this Url
	std::string getIcon() const { return mIcon; }

	/// Return the color to render the displayed text
	LLUIColor getColor() const { return mColor; }

	/// Return the name of a XUI file containing the context menu items
	std::string getMenuName() const { return mMenuName; }

	/// return the SL location that this Url describes, or "" if none.
	std::string getLocation() const { return mLocation; }

	/// is this a match for a URL that should not be hyperlinked?
	bool isLinkDisabled() const { return mDisabledLink; }

	/// Change the contents of this match object (used by LLUrlRegistry)
	void setValues(U32 start, U32 end, const std::string &url, const std::string &label,
	               const std::string &tooltip, const std::string &icon,
				   const LLUIColor& color, const std::string &menu, 
				   const std::string &location, bool disabled_link);

private:
	U32         mStart;
	U32         mEnd;
	std::string mUrl;
	std::string mLabel;
	std::string mTooltip;
	std::string mIcon;
	std::string mMenuName;
	std::string mLocation;
	LLUIColor	mColor;
	bool        mDisabledLink;
};

#endif
