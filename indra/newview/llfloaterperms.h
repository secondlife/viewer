/** 
 * @file llfloaterperms.h
 * @brief Asset creation permission preferences.
 * @author Coco 
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLFLOATERPERMPREFS_H
#define LL_LLFLOATERPERMPREFS_H

#include "llfloater.h"

class LLFloaterPerms : public LLFloater
{
	friend class LLFloaterReg;
	
public:
	/*virtual*/ BOOL postBuild();

	// Convenience methods to get current permission preference bitfields from saved settings:
	static U32 getEveryonePerms(std::string prefix=""); // prefix + "EveryoneCopy"
	static U32 getGroupPerms(std::string prefix=""); // prefix + "ShareWithGroup"
	static U32 getNextOwnerPerms(std::string prefix=""); // bitfield for prefix + "NextOwner" + "Copy", "Modify", and "Transfer"
	static U32 getNextOwnerPermsInverted(std::string prefix="");

private:
	LLFloaterPerms(const LLSD& seed);

};

class LLFloaterPermsDefault : public LLFloater
{
	friend class LLFloaterReg;

public:
	/*virtual*/ BOOL postBuild();
	void ok();
	void cancel();
	void onClickOK();
	void onClickCancel();
	void onCommitCopy(const LLSD& user_data);

enum Categories
{
	CAT_OBJECTS,
	CAT_UPLOADS,
	CAT_SCRIPTS,
	CAT_NOTECARDS,
	CAT_GESTURES,
	CAT_WEARABLES,
	CAT_LAST
};

private:
	LLFloaterPermsDefault(const LLSD& seed);
	void refresh();

	std::string category_names[CAT_LAST];

	// cached values only for implementing cancel.
	bool mShareWithGroup[CAT_LAST];
	bool mEveryoneCopy[CAT_LAST];
	bool mNextOwnerCopy[CAT_LAST];
	bool mNextOwnerModify[CAT_LAST];
	bool mNextOwnerTransfer[CAT_LAST];
};

#endif
