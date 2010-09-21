/**
 * @file llfloaterwhitelistentry.cpp
 * @brief LLFloaterWhistListEntry class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llfloaterreg.h"
#include "llfloatermediasettings.h"
#include "llfloaterwhitelistentry.h"
#include "llpanelmediasettingssecurity.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "lllineeditor.h"


///////////////////////////////////////////////////////////////////////////////
//
LLFloaterWhiteListEntry::LLFloaterWhiteListEntry( const LLSD& key ) :
	LLFloater(key)
{
//	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_whitelist_entry.xml");
}

///////////////////////////////////////////////////////////////////////////////
//
LLFloaterWhiteListEntry::~LLFloaterWhiteListEntry()
{
}

///////////////////////////////////////////////////////////////////////////////
//
BOOL LLFloaterWhiteListEntry::postBuild()
{
	mWhiteListEdit = getChild<LLLineEditor>("whitelist_entry");

	childSetAction("cancel_btn", onBtnCancel, this);
	childSetAction("ok_btn", onBtnOK, this);

	setDefaultBtn("ok_btn");

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// static
void LLFloaterWhiteListEntry::onBtnOK( void* userdata )
{
	LLFloaterWhiteListEntry *self =(LLFloaterWhiteListEntry *)userdata;

	LLPanelMediaSettingsSecurity* panel = LLFloaterReg::getTypedInstance<LLFloaterMediaSettings>("media_settings")->getPanelSecurity();
	if ( panel )
	{
		std::string white_list_item = self->mWhiteListEdit->getText();

		panel->addWhiteListEntry( white_list_item );
		panel->updateWhitelistEnableStatus();
	};
	
	self->closeFloater();	
}

///////////////////////////////////////////////////////////////////////////////
// static
void LLFloaterWhiteListEntry::onBtnCancel( void* userdata )
{
	LLFloaterWhiteListEntry *self =(LLFloaterWhiteListEntry *)userdata;
	
	self->closeFloater();
}
