/** 
* @file llsidetraypanelcontainer.h
* @brief LLSideTrayPanelContainer class declaration
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLSIDETRAY_PANEL_CONTAINER_H
#define LL_LLSIDETRAY_PANEL_CONTAINER_H

#include "lltabcontainer.h"

/**
* LLSideTrayPanelContainer class acts like LLTabContainer with invisible tabs.
* It is designed to make panel switching easier, avoid setVisible(TRUE) setVisible(FALSE)
* calls and related workarounds.
* use onOpen to open sub panel, pass the name of panel to open
* in key[PARAM_SUB_PANEL_NAME].
* LLSideTrayPanelContainer also implements panel navigation history - it allows to 
* open previous or next panel if navigation history is available(after user 
* has opened two or more panels). *NOTE - only back navigation is implemented so far.
*/
class LLSideTrayPanelContainer : public LLTabContainer
{
public:

	struct Params :	public LLInitParam::Block<Params, LLTabContainer::Params>
	{
		Optional<std::string> default_panel_name;
		Params();
	};

	/**
	* Opens sub panel
	* @param key - params to be passed to panel, use key[PARAM_SUB_PANEL_NAME]
	* to specify panel name to be opened.
	*/
	/*virtual*/ void onOpen(const LLSD& key);

	/**
	 * Opens given subpanel.
	 */
	void openPanel(const std::string& panel_name, const LLSD& key = LLSD::emptyMap());

	/**
	* Opens previous panel from panel navigation history.
	*/
	void openPreviousPanel();

	/**
	* Overrides LLTabContainer::handleKeyHere to disable panel switch on 
	* Alt + Left/Right button press.
	*/
	BOOL handleKeyHere(KEY key, MASK mask);

	/**
	* Name of parameter that stores panel name to open.
	*/
	static std::string PARAM_SUB_PANEL_NAME;

protected:
	LLSideTrayPanelContainer(const Params& p);
	friend class LLUICtrlFactory;

	/**
	* std::string - name of panel
	* S32 - index of previous panel
	* *NOTE - no forward navigation implemented yet
	*/
	typedef std::map<std::string, S32> panel_navigation_history_t;

	// Navigation history
	panel_navigation_history_t mPanelHistory;
	std::string mDefaultPanelName;
};

#endif //LL_LLSIDETRAY_PANEL_CONTAINER_H
