/** 
 * @file llbottomtray.cpp
 * @brief LLBottomTray class implementation
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

#include "llviewerprecompiledheaders.h" // must be first include
#include "llbottomtray.h"

#include "llagent.h"
#include "llchiclet.h"
#include "llfloaterreg.h"
#include "llflyoutbutton.h"
#include "lllayoutstack.h"
#include "llnearbychatbar.h"
#include "llsplitbutton.h"
#include "llfloatercamera.h"
#include "llimpanel.h"

LLBottomTray::LLBottomTray(const LLSD&)
:	mChicletPanel(NULL),
	mIMWell(NULL),
	mSysWell(NULL),
	mTalkBtn(NULL),
	mNearbyChatBar(NULL),
	mToolbarStack(NULL)

{
	mFactoryMap["chat_bar"] = LLCallbackMap(LLBottomTray::createNearbyChatBar, NULL);

	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_bottomtray.xml");

	mChicletPanel = getChild<LLChicletPanel>("chiclet_list");
	mIMWell = getChild<LLNotificationChiclet>("im_well");
	mSysWell = getChild<LLNotificationChiclet>("sys_well");

	mSysWell->setNotificationChicletWindow(LLFloaterReg::getInstance("syswell_window"));
	mChicletPanel->setChicletClickedCallback(boost::bind(&LLBottomTray::onChicletClick,this,_1));

	LLSplitButton* presets = getChild<LLSplitButton>("presets");
	presets->setSelectionCallback(LLFloaterCamera::onClickCameraPresets);

	LLIMMgr::getInstance()->addSessionObserver(this);

	//this is to fix a crash that occurs because LLBottomTray is a singleton
	//and thus is deleted at the end of the viewers lifetime, but to be cleanly
	//destroyed LLBottomTray requires some subsystems that are long gone
	LLUI::getRootView()->addChild(this);

	// Necessary for focus movement among child controls
	setFocusRoot(TRUE);
}

BOOL LLBottomTray::postBuild()
{
	mNearbyChatBar = getChild<LLNearbyChatBar>("chat_bar");
	mToolbarStack = getChild<LLLayoutStack>("toolbar_stack");

	return TRUE;
}

LLBottomTray::~LLBottomTray()
{
	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}
}

void LLBottomTray::onChicletClick(LLUICtrl* ctrl)
{
	LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(ctrl);
	if (chiclet)
	{
		// Until you can type into an IM Window and have a conversation,
		// still show the old communicate window
		//LLFloaterReg::showInstance("communicate", chiclet->getSessionId());

		// Show after comm window so it is frontmost (and hence will not
		// auto-hide)
		LLIMFloater::show(chiclet->getSessionId());
		chiclet->setCounter(0);
	}
}

void* LLBottomTray::createNearbyChatBar(void* userdata)
{
	return new LLNearbyChatBar();
}

//virtual
void LLBottomTray::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	if(getChicletPanel())
	{
		if(getChicletPanel()->findChiclet<LLChiclet>(session_id))
		{

		}
		else
		{
			LLIMChiclet* chiclet = getChicletPanel()->createChiclet<LLIMChiclet>(session_id);
			chiclet->setIMSessionName(name);
			chiclet->setOtherParticipantId(other_participant_id);
		}
	}
}

//virtual
void LLBottomTray::sessionRemoved(const LLUUID& session_id)
{
	if(getChicletPanel())
	{
		getChicletPanel()->removeChiclet(session_id);
	}
}

//virtual
void LLBottomTray::onFocusLost()
{
	if (gAgent.cameraMouselook())
	{
		setVisible(FALSE);
	}
}

//virtual
// setVisible used instead of onVisibilityChange, since LLAgent calls it on entering/leaving mouselook mode.
// If bottom tray is already visible in mouselook mode, then onVisibilityChange will not be called from setVisible(true),
void LLBottomTray::setVisible(BOOL visible)
{
	LLPanel::setVisible(visible);

	// *NOTE: we must check mToolbarStack against NULL because sewtVisible is called from the 
	// LLPanel::initFromParams BEFORE postBuild is called and child controls are not exist yet
	if (NULL != mToolbarStack)
	{
		BOOL visibility = gAgent.cameraMouselook() ? false : true;

		for ( child_list_const_iter_t child_it = mToolbarStack->getChildList()->begin(); 
			child_it != mToolbarStack->getChildList()->end(); child_it++)
		{
			LLView* viewp = *child_it;
			
			if ("chat_bar" == viewp->getName())
				continue;
			else 
			{
				viewp->setVisible(visibility);
			}
		}
	}
}
