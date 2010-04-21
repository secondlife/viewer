/** 
 * @file llavatarname.h
 * @brief Represents name-related data for an avatar, such as the
 * username/SLID ("bobsmith123" or "james.linden") and the display
 * name ("James Cook")
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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
#ifndef LLAVATARNAME_H
#define LLAVATARNAME_H

#include <string>

class LLSD;

class LL_COMMON_API LLAvatarName
{
public:
	LLAvatarName();
	
	bool operator<(const LLAvatarName& rhs) const;

	LLSD asLLSD() const;

	void fromLLSD(const LLSD& sd);

	// "bobsmith123" or "james.linden", US-ASCII only
	std::string mSLID;

	// "Jose' Sanchez" or "James Linden", UTF-8 encoded Unicode
	// Contains data whether or not user has explicitly set
	// a display name; may duplicate their SLID.
	std::string mDisplayName;

	// If true, both display name and SLID were generated from
	// a legacy first and last name, like "James Linden (james.linden)"
	bool mIsDisplayNameDefault;

	// Names can change, so need to keep track of when name was
	// last checked.
	// Unix time-from-epoch seconds
	F64 mExpires;

	// Can be a viewer UI image name ("Person_Check") or a server-side
	// image UUID, or empty string.
	std::string mBadge;
};

#endif
