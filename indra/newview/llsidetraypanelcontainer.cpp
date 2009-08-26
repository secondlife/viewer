/** 
* @file llsidetraypanelcontainer.cpp
* @brief LLSideTrayPanelContainer implementation
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
#include "llsidetraypanelcontainer.h"

static LLDefaultChildRegistry::Register<LLSideTrayPanelContainer> r2("panel_container");

std::string LLSideTrayPanelContainer::PARAM_SUB_PANEL_NAME = "sub_panel_name";

LLSideTrayPanelContainer::Params::Params()
{
	// Always hide tabs.
	hide_tabs(true);
}

LLSideTrayPanelContainer::LLSideTrayPanelContainer(const Params& p)
 : LLTabContainer(p)
{
}

void LLSideTrayPanelContainer::onOpen(const LLSD& key)
{
	// Select specified panel and save navigation history.
	if(key.has(PARAM_SUB_PANEL_NAME))
	{
		// Save panel navigation history
		std::string panel_name = key[PARAM_SUB_PANEL_NAME];
		S32 old_index = getCurrentPanelIndex();

		selectTabByName(panel_name);

		S32 new_index = getCurrentPanelIndex();

		// Don't update navigation history if we are opening same panel again.
		if(old_index != new_index)
		{
			mPanelHistory[panel_name] = old_index;
		}
	}
	// Will reopen current panel if no panel name was passed.
	getCurrentPanel()->onOpen(key);
}

void LLSideTrayPanelContainer::openPreviousPanel()
{
	std::string current_panel_name = getCurrentPanel()->getName();
	panel_navigation_history_t::const_iterator it = mPanelHistory.find(current_panel_name);
	if(mPanelHistory.end() != it)
	{
		selectTab(it->second);
	}
}

BOOL LLSideTrayPanelContainer::handleKeyHere(KEY key, MASK mask)
{
	// No key press handling code for Panel Container - this disables
	// Tab Container's Alt + Left/Right Button tab switching.
	
	// Let default handler process key presses, don't simply return TRUE or FALSE
	// as this may brake some functionality as it did with Copy/Paste for 
	// text_editor (ticket EXT-642).
	return LLPanel::handleKeyHere(key, mask);
}
