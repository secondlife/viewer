/** 
 * @file llinspectavatar.h
 * @brief Avatar Inspector, a small information window used when clicking
 * on avatar names in the 2D UI and in the ambient inspector widget for
 * the 3D world.
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

#ifndef LLINSPECTAVATAR_H
#define LLINSPECTAVATAR_H

#include "llfloater.h"

class LLInspectAvatar : public LLFloater
{
	friend class LLFloaterReg;

public:
	// key is the UUID of avatar for whom to show information
	// *TODO: Needs to take a spawn location
	LLInspectAvatar(const LLSD& avatar_id);
	virtual ~LLInspectAvatar();

	/*virtual*/ BOOL postBuild(void);

	void setAvatarID(const LLUUID &avatar_id);

	const LLUUID&		getAvatarID() const	{ return mAvatarID; }
	const std::string&	getFirstName() const { return mFirstName; }
	const std::string&	getLastName() const { return mLastName; }

private:
	// Update widgets, including avatar name, buttons enabled, etc.
	// Used after avatar id changes.
	void refresh();

	void onClickAddFriend();
	void onClickViewProfile();

	void nameUpdatedCallback(
		const LLUUID& id,
		const std::string& first,
		const std::string& last,
		BOOL is_group);

private:
	LLUUID				mAvatarID;
	// Need avatar name information to spawn friend add request
	std::string			mFirstName;
	std::string			mLastName;
};


#endif
