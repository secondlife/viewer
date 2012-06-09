/** 
 * @file llfloaterperms.cpp
 * @brief Asset creation permission preferences.
 * @author Coco
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llcheckboxctrl.h"
#include "llfloaterperms.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llpermissions.h"


LLFloaterPerms::LLFloaterPerms(const LLSD& seed)
: LLFloater(seed)
{
}

BOOL LLFloaterPerms::postBuild()
{
	return true;
}

//static 
U32 LLFloaterPerms::getGroupPerms(std::string prefix)
{	
	return gSavedSettings.getBOOL(prefix+"ShareWithGroup") ? PERM_COPY : PERM_NONE;
}

//static 
U32 LLFloaterPerms::getEveryonePerms(std::string prefix)
{
	return gSavedSettings.getBOOL(prefix+"EveryoneCopy") ? PERM_COPY : PERM_NONE;
}

//static 
U32 LLFloaterPerms::getNextOwnerPerms(std::string prefix)
{
	U32 flags = PERM_MOVE;
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerCopy") )
	{
		flags |= PERM_COPY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerModify") )
	{
		flags |= PERM_MODIFY;
	}
	if ( gSavedSettings.getBOOL(prefix+"NextOwnerTransfer") )
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}

//static 
U32 LLFloaterPerms::getNextOwnerPermsInverted(std::string prefix)
{
	// Sets bits for permissions that are off
	U32 flags = PERM_MOVE;
	if ( !gSavedSettings.getBOOL(prefix+"NextOwnerCopy") )
	{
		flags |= PERM_COPY;
	}
	if ( !gSavedSettings.getBOOL(prefix+"NextOwnerModify") )
	{
		flags |= PERM_MODIFY;
	}
	if ( !gSavedSettings.getBOOL(prefix+"NextOwnerTransfer") )
	{
		flags |= PERM_TRANSFER;
	}
	return flags;
}

LLFloaterPermsDefault::LLFloaterPermsDefault(const LLSD& seed)
: LLFloater(seed)
{
	mCommitCallbackRegistrar.add("PermsDefault.Copy", boost::bind(&LLFloaterPermsDefault::onCommitCopy, this, _2));
	mCommitCallbackRegistrar.add("PermsDefault.OK", boost::bind(&LLFloaterPermsDefault::onClickOK, this));
	mCommitCallbackRegistrar.add("PermsDefault.Cancel", boost::bind(&LLFloaterPermsDefault::onClickCancel, this));
}

BOOL LLFloaterPermsDefault::postBuild()
{
	mCloseSignal.connect(boost::bind(&LLFloaterPermsDefault::cancel, this));

	category_names[CAT_OBJECTS] = "Objects";
	category_names[CAT_UPLOADS] = "Uploads";
	category_names[CAT_SCRIPTS] = "Scripts";
	category_names[CAT_NOTECARDS] = "Notecards";
	category_names[CAT_GESTURES] = "Gestures";
	category_names[CAT_WEARABLES] = "Wearables";

	refresh();
	
	return true;
}

void LLFloaterPermsDefault::onClickOK()
{
	ok();
	closeFloater();
}

void LLFloaterPermsDefault::onClickCancel()
{
	cancel();
	closeFloater();
}

void LLFloaterPermsDefault::onCommitCopy(const LLSD& user_data)
{
	// Implements fair use
	std::string prefix = user_data.asString();

	BOOL copyable = gSavedSettings.getBOOL(prefix+"NextOwnerCopy");
	if(!copyable)
	{
		gSavedSettings.setBOOL(prefix+"NextOwnerTransfer", TRUE);
	}
	LLCheckBoxCtrl* xfer = getChild<LLCheckBoxCtrl>(prefix+"_transfer");
	xfer->setEnabled(copyable);
}

void LLFloaterPermsDefault::ok()
{
	refresh(); // Changes were already applied to saved settings. Refreshing internal values makes it official.
}

void LLFloaterPermsDefault::cancel()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		gSavedSettings.setBOOL(category_names[iter]+"NextOwnerCopy",		mNextOwnerCopy[iter]);
		gSavedSettings.setBOOL(category_names[iter]+"NextOwnerModify",		mNextOwnerModify[iter]);
		gSavedSettings.setBOOL(category_names[iter]+"NextOwnerTransfer",	mNextOwnerTransfer[iter]);
		gSavedSettings.setBOOL(category_names[iter]+"ShareWithGroup",		mShareWithGroup[iter]);
		gSavedSettings.setBOOL(category_names[iter]+"EveryoneCopy",			mEveryoneCopy[iter]);
	}
}

void LLFloaterPermsDefault::refresh()
{
	for (U32 iter = CAT_OBJECTS; iter < CAT_LAST; iter++)
	{
		mShareWithGroup[iter]    = gSavedSettings.getBOOL(category_names[iter]+"ShareWithGroup");
		mEveryoneCopy[iter]      = gSavedSettings.getBOOL(category_names[iter]+"EveryoneCopy");
		mNextOwnerCopy[iter]     = gSavedSettings.getBOOL(category_names[iter]+"NextOwnerCopy");
		mNextOwnerModify[iter]   = gSavedSettings.getBOOL(category_names[iter]+"NextOwnerModify");
		mNextOwnerTransfer[iter] = gSavedSettings.getBOOL(category_names[iter]+"NextOwnerTransfer");
	}
}
