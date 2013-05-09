/** 
* @file llsidetraypanelcontainer.cpp
* @brief LLSideTrayPanelContainer implementation
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
#include "llsidetraypanelcontainer.h"

static LLDefaultChildRegistry::Register<LLSideTrayPanelContainer> r2("panel_container");

std::string LLSideTrayPanelContainer::PARAM_SUB_PANEL_NAME = "sub_panel_name";

LLSideTrayPanelContainer::Params::Params()
:	default_panel_name("default_panel_name")
{
	// Always hide tabs.
	changeDefault(hide_tabs, true);
}

LLSideTrayPanelContainer::LLSideTrayPanelContainer(const Params& p)
 : LLTabContainer(p)
 , mDefaultPanelName(p.default_panel_name)
{
}

void LLSideTrayPanelContainer::onOpen(const LLSD& key)
{
	// Select specified panel and save navigation history.
	if(key.has(PARAM_SUB_PANEL_NAME))
	{
		//*NOTE dzaporozhan
		// Navigation history is not used after fix for EXT-3186,
		// openPreviousPanel() always opens default panel

		// Save panel navigation history
		std::string panel_name = key[PARAM_SUB_PANEL_NAME];

		selectTabByName(panel_name);
	}
	// Will reopen current panel if no panel name was passed.
	getCurrentPanel()->onOpen(key);
}

void LLSideTrayPanelContainer::openPanel(const std::string& panel_name, const LLSD& key)
{
	LLSD combined_key = key;
	combined_key[PARAM_SUB_PANEL_NAME] = panel_name;
	onOpen(combined_key);
}

void LLSideTrayPanelContainer::openPreviousPanel()
{
	if(!mDefaultPanelName.empty())
	{
		selectTabByName(mDefaultPanelName);
	}
	else
	{
		selectTab(0);
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
