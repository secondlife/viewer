/** 
 * @file llavatarname.cpp
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
#include "linden_common.h"

#include "llavatarname.h"

#include "lldate.h"
#include "llsd.h"

// Store these in pre-built std::strings to avoid memory allocations in
// LLSD map lookups
static const std::string SL_ID("sl_id");
static const std::string DISPLAY_NAME("display_name");
static const std::string LEGACY_FIRST_NAME("legacy_first_name");
static const std::string LEGACY_LAST_NAME("legacy_last_name");
static const std::string IS_DISPLAY_NAME_DEFAULT("is_display_name_default");
static const std::string DISPLAY_NAME_EXPIRES("display_name_expires");

LLAvatarName::LLAvatarName()
:	mUsername(),
	mDisplayName(),
	mLegacyFirstName(),
	mLegacyLastName(),
	mIsDisplayNameDefault(false),
	mIsDummy(false),
	mExpires(F64_MAX)
{ }

bool LLAvatarName::operator<(const LLAvatarName& rhs) const
{
	if (mUsername == rhs.mUsername)
		return mDisplayName < rhs.mDisplayName;
	else
		return mUsername < rhs.mUsername;
}

LLSD LLAvatarName::asLLSD() const
{
	LLSD sd;
	// Due to a late-breaking change request from Product, we renamed
	// "SLID" to "Username", but it was too late to change the wire format.
	sd[SL_ID] = mUsername;
	sd[DISPLAY_NAME] = mDisplayName;
	sd[LEGACY_FIRST_NAME] = mLegacyFirstName;
	sd[LEGACY_LAST_NAME] = mLegacyLastName;
	sd[IS_DISPLAY_NAME_DEFAULT] = mIsDisplayNameDefault;
	sd[DISPLAY_NAME_EXPIRES] = LLDate(mExpires);
	return sd;
}

void LLAvatarName::fromLLSD(const LLSD& sd)
{
	mUsername = sd[SL_ID].asString(); // see asLLSD() above
	mDisplayName = sd[DISPLAY_NAME].asString();
	mLegacyFirstName = sd[LEGACY_FIRST_NAME].asString();
	mLegacyLastName = sd[LEGACY_LAST_NAME].asString();
	mIsDisplayNameDefault = sd[IS_DISPLAY_NAME_DEFAULT].asBoolean();
	LLDate expires = sd[DISPLAY_NAME_EXPIRES];
	mExpires = expires.secondsSinceEpoch();
}

std::string LLAvatarName::getNameAndSLID() const
{
	std::string name;
	if (!mUsername.empty())
	{
		name = mDisplayName + " (" + mUsername + ")";
	}
	else
	{
		// ...display names are off, legacy name is in mDisplayName
		name = mDisplayName;
	}
	return name;
}

std::string LLAvatarName::getLegacyName() const
{
	std::string name;
	name.reserve( mLegacyFirstName.size() + 1 + mLegacyLastName.size() );
	name = mLegacyFirstName;
	name += " ";
	name += mLegacyLastName;
	return name;
}
