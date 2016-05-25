/**
 * @file llfloaterwhitelistentry.cpp
 * @brief LLFloaterWhistListEntry class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
