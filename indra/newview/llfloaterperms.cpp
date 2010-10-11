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
	mCommitCallbackRegistrar.add("Perms.Copy",	boost::bind(&LLFloaterPerms::onCommitCopy, this));
	mCommitCallbackRegistrar.add("Perms.OK",	boost::bind(&LLFloaterPerms::onClickOK, this));
	mCommitCallbackRegistrar.add("Perms.Cancel",	boost::bind(&LLFloaterPerms::onClickCancel, this));

}

BOOL LLFloaterPerms::postBuild()
{
	mCloseSignal.connect(boost::bind(&LLFloaterPerms::cancel, this));
	
	refresh();
	
	return TRUE;
}

void LLFloaterPerms::onClickOK()
{
	ok();
	closeFloater();
}

void LLFloaterPerms::onClickCancel()
{
	cancel();
	closeFloater();
}

void LLFloaterPerms::onCommitCopy()
{
	// Implements fair use
	BOOL copyable = gSavedSettings.getBOOL("NextOwnerCopy");
	if(!copyable)
	{
		gSavedSettings.setBOOL("NextOwnerTransfer", TRUE);
	}
	LLCheckBoxCtrl* xfer = getChild<LLCheckBoxCtrl>("next_owner_transfer");
	xfer->setEnabled(copyable);
}

void LLFloaterPerms::ok()
{
	refresh(); // Changes were already applied to saved settings. Refreshing internal values makes it official.
}

void LLFloaterPerms::cancel()
{
	gSavedSettings.setBOOL("ShareWithGroup",    mShareWithGroup);
	gSavedSettings.setBOOL("EveryoneCopy",      mEveryoneCopy);
	gSavedSettings.setBOOL("NextOwnerCopy",     mNextOwnerCopy);
	gSavedSettings.setBOOL("NextOwnerModify",   mNextOwnerModify);
	gSavedSettings.setBOOL("NextOwnerTransfer", mNextOwnerTransfer);
}

void LLFloaterPerms::refresh()
{
	mShareWithGroup    = gSavedSettings.getBOOL("ShareWithGroup");
	mEveryoneCopy      = gSavedSettings.getBOOL("EveryoneCopy");
	mNextOwnerCopy     = gSavedSettings.getBOOL("NextOwnerCopy");
	mNextOwnerModify   = gSavedSettings.getBOOL("NextOwnerModify");
	mNextOwnerTransfer = gSavedSettings.getBOOL("NextOwnerTransfer");
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

