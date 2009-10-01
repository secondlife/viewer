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
#include "llimfloater.h" // for LLIMFloater
#include "lllayoutstack.h"
#include "llnearbychatbar.h"
#include "llsplitbutton.h"
#include "llsyswellwindow.h"
#include "llfloatercamera.h"

LLBottomTray::LLBottomTray(const LLSD&)
:	mChicletPanel(NULL),
	mSysWell(NULL),
	mTalkBtn(NULL),
	mNearbyChatBar(NULL),
	mToolbarStack(NULL)

{
	mFactoryMap["chat_bar"] = LLCallbackMap(LLBottomTray::createNearbyChatBar, NULL);

	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_bottomtray.xml");

	mChicletPanel = getChild<LLChicletPanel>("chiclet_list");
	mSysWell = getChild<LLNotificationChiclet>("sys_well");

	// init mSysWell
	// set handler for a Click operation
	mSysWell->setClickCallback(boost::bind(&LLSysWellWindow::onChicletClick, LLFloaterReg::getTypedInstance<LLSysWellWindow>("syswell_window")));

	mChicletPanel->setChicletClickedCallback(boost::bind(&LLBottomTray::onChicletClick,this,_1));

	LLUICtrl::CommitCallbackRegistry::defaultRegistrar().add("CameraPresets.ChangeView",&LLFloaterCameraPresets::onClickCameraPresets);
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
	mBottomTrayContextMenu =  LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_bottomtray.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	gMenuHolder->addChild(mBottomTrayContextMenu);

	mNearbyChatBar = getChild<LLNearbyChatBar>("chat_bar");
	mToolbarStack = getChild<LLLayoutStack>("toolbar_stack");
	mMovementPanel = getChild<LLPanel>("movement_panel");
	mGestureCombo = getChild<LLComboBox>("Gesture");
	mCamPanel = getChild<LLPanel>("cam_panel");
	setRightMouseDownCallback(boost::bind(&LLBottomTray::showBottomTrayContextMenu,this, _2, _3,_4));

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

// this logic has been moved to LLIMChiclet::handleMouseDown
//		LLIMFloater::show(chiclet->getSessionId());
//		chiclet->setCounter(0);
	}
}

// *TODO Vadim: why void* ?
void* LLBottomTray::createNearbyChatBar(void* userdata)
{
	return new LLNearbyChatBar();
}

LLIMChiclet* LLBottomTray::createIMChiclet(const LLUUID& session_id)
{
	LLIMChiclet::EType im_chiclet_type = LLIMChiclet::getIMSessionType(session_id);

	switch (im_chiclet_type)
	{
	case LLIMChiclet::TYPE_IM:
		return getChicletPanel()->createChiclet<LLIMP2PChiclet>(session_id);
	case LLIMChiclet::TYPE_GROUP:
	case LLIMChiclet::TYPE_AD_HOC:
		return getChicletPanel()->createChiclet<LLIMGroupChiclet>(session_id);
	case LLIMChiclet::TYPE_UNKNOWN:
		break;
	}

	return NULL;
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
			LLIMChiclet* chiclet = createIMChiclet(session_id);
			if(chiclet)
			{
				chiclet->setIMSessionName(name);
				chiclet->setOtherParticipantId(other_participant_id);
			}
			else
			{
				llerrs << "Could not create chiclet" << llendl;
			}
		}
	}
}

//virtual
void LLBottomTray::sessionRemoved(const LLUUID& session_id)
{
	if(getChicletPanel())
	{
		// IM floater should be closed when session removed and associated chiclet closed
		LLIMFloater* iMfloater = LLFloaterReg::findTypedInstance<LLIMFloater>(
				"impanel", session_id);
		if (iMfloater != NULL)
		{
			iMfloater->closeFloater();
		}

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

	// *NOTE: we must check mToolbarStack against NULL because setVisible is called from the 
	// LLPanel::initFromParams BEFORE postBuild is called and child controls are not exist yet
	if (NULL != mToolbarStack)
	{
		BOOL visibility = gAgent.cameraMouselook() ? false : true;

		for ( child_list_const_iter_t child_it = mToolbarStack->getChildList()->begin(); 
			child_it != mToolbarStack->getChildList()->end(); child_it++)
		{
			LLView* viewp = *child_it;
			std::string name = viewp->getName();
			
			if ("chat_bar" == name || "movement_panel" == name || "cam_panel" == name)
				continue;
			else 
			{
				viewp->setVisible(visibility);
			}
		}
	}
}

void LLBottomTray::showBottomTrayContextMenu(S32 x, S32 y, MASK mask)
{
	// We should show BottomTrayContextMenu in last  turn
	if (mBottomTrayContextMenu && !LLMenuGL::sMenuContainer->getVisibleMenu())
	{
		//there are no other context menu (IM chiclet etc ), so we can show BottomTrayContextMenu
			mBottomTrayContextMenu->buildDrawLabels();
			mBottomTrayContextMenu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(this, mBottomTrayContextMenu, x, y);
		
	}
}

void LLBottomTray::showGestureButton(BOOL visible)
{
	if (visible != mGestureCombo->getVisible())
	{
		LLRect r = mNearbyChatBar->getRect();

		mGestureCombo->setVisible(visible);

		if (!visible)
		{
			LLFloaterReg::hideFloaterInstance("gestures");
			r.mRight -= mGestureCombo->getRect().getWidth();
		}
		else
		{
			r.mRight += mGestureCombo->getRect().getWidth();
		}

		mNearbyChatBar->setRect(r);
	}
}

void LLBottomTray::showMoveButton(BOOL visible)
{
	mMovementPanel->setVisible(visible);
}

void LLBottomTray::showCameraButton(BOOL visible)
{
	mCamPanel->setVisible(visible);
}
