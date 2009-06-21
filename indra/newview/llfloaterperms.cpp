/** 
 * @file llfloaterperms.cpp
 * @brief Asset creation permission preferences.
 * @author Coco
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llalertdialog.h"
#include "llcheckboxctrl.h"
#include "llfloaterperms.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "lluictrlfactory.h"
#include "llpermissions.h"


LLFloaterPerms::LLFloaterPerms(const LLSD& seed)
: LLFloater()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_perm_prefs.xml");
}

BOOL LLFloaterPerms::postBuild()
{
	childSetEnabled("next_owner_transfer", gSavedSettings.getBOOL("NextOwnerCopy"));
	childSetAction("help",   onClickHelp,   this);
	childSetAction("ok",     onClickOK,     this);
	childSetAction("cancel", onClickCancel, this);
	childSetCommitCallback("next_owner_copy", &onCommitCopy, this);

	refresh();
	
	return TRUE;
}

//static 
void LLFloaterPerms::onClickOK(void* data)
{
	LLFloaterPerms* self = static_cast<LLFloaterPerms*>(data);
	self->ok();
	self->closeFloater();
}

//static 
void LLFloaterPerms::onClickCancel(void* data)
{
	LLFloaterPerms* self = static_cast<LLFloaterPerms*>(data);
	self->cancel();
	self->closeFloater();
}

//static 
void LLFloaterPerms::onCommitCopy(LLUICtrl* ctrl, void* data)
{
	LLFloaterPerms* self = static_cast<LLFloaterPerms*>(data);
	// Implements fair use
	BOOL copyable = gSavedSettings.getBOOL("NextOwnerCopy");
	if(!copyable)
	{
		gSavedSettings.setBOOL("NextOwnerTransfer", TRUE);
	}
	LLCheckBoxCtrl* xfer = self->getChild<LLCheckBoxCtrl>("next_owner_transfer");
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

void LLFloaterPerms::onClose(bool app_quitting)
{
	// Cancel any unsaved changes before closing. 
	// Note: when closed due to the OK button this amounts to a no-op.
	cancel();
	destroy();
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
	U32 flags = 0;
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
void LLFloaterPerms::onClickHelp(void* data)
{
	LLNotifications::instance().add("ClickUploadHelpPermissions");
}
