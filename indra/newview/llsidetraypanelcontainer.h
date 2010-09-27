/** 
* @file llsidetraypanelcontainer.h
* @brief LLSideTrayPanelContainer class declaration
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
