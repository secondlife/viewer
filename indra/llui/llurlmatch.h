/** 
 * @file llurlmatch.h
 * @author Martin Reddy
 * @brief Specifies a matched Url in a string, as returned by LLUrlRegistry
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

#ifndef LL_LLURLMATCH_H
#define LL_LLURLMATCH_H

//#include "linden_common.h"

#include <string>
#include <vector>
#include "llstyle.h"

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
	LLStyle::Params getStyle() const { return mStyle; }

	/// Return the name of a XUI file containing the context menu items
	std::string getMenuName() const { return mMenuName; }

	/// return the SL location that this Url describes, or "" if none.
	std::string getLocation() const { return mLocation; }

	/// Should this link text be underlined only when mouse is hovered over it?
	bool underlineOnHoverOnly() const { return mUnderlineOnHoverOnly; }

	/// Change the contents of this match object (used by LLUrlRegistry)
	void setValues(U32 start, U32 end, const std::string &url, const std::string &label,
	               const std::string &tooltip, const std::string &icon,
				   const LLStyle::Params& style, const std::string &menu, 
				   const std::string &location, const LLUUID& id,
				   bool underline_on_hover_only = false );

	const LLUUID& getID() const { return mID; }
private:
	U32         mStart;
	U32         mEnd;
	std::string mUrl;
	std::string mLabel;
	std::string mTooltip;
	std::string mIcon;
	std::string mMenuName;
	std::string mLocation;
	LLUUID		mID;
	LLStyle::Params mStyle;
	bool		mUnderlineOnHoverOnly;
};

#endif
