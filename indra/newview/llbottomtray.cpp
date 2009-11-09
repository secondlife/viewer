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
	mSpeakBtn(NULL),
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

	LLUICtrl::CommitCallbackRegistry::defaultRegistrar().add("CameraPresets.ChangeView", boost::bind(&LLFloaterCamera::onClickCameraPresets, _2));
	LLIMMgr::getInstance()->addSessionObserver(this);

	//this is to fix a crash that occurs because LLBottomTray is a singleton
	//and thus is deleted at the end of the viewers lifetime, but to be cleanly
	//destroyed LLBottomTray requires some subsystems that are long gone
	LLUI::getRootView()->addChild(this);

	// Necessary for focus movement among child controls
	setFocusRoot(TRUE);
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
		return getChicletPanel()->createChiclet<LLIMGroupChiclet>(session_id);
	case LLIMChiclet::TYPE_AD_HOC:
		return getChicletPanel()->createChiclet<LLAdHocChiclet>(session_id);
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

void LLBottomTray::sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	//this is only needed in case of outgoing ad-hoc/group chat sessions
	LLChicletPanel* chiclet_panel = getChicletPanel();
	if (chiclet_panel)
	{
		//it should be ad-hoc im chiclet or group im chiclet
		LLChiclet* chiclet = chiclet_panel->findChiclet<LLChiclet>(old_session_id);
		if (chiclet) chiclet->setSessionId(new_session_id);
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
			
			if ("chat_bar" == name || "movement_panel" == name || "cam_panel" == name || "snapshot_panel" == name)
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
	mGesturePanel->setVisible(visible);
}

void LLBottomTray::showMoveButton(BOOL visible)
{
	mMovementPanel->setVisible(visible);
}

void LLBottomTray::showCameraButton(BOOL visible)
{
	mCamPanel->setVisible(visible);
}

void LLBottomTray::showSnapshotButton(BOOL visible)
{
	mSnapshotPanel->setVisible(visible);
}

namespace
{
	const std::string& PANEL_CHICLET_NAME = "chiclet_list_panel";
	const std::string& PANEL_CHATBAR_NAME = "chat_bar";
	const std::string& PANEL_MOVEMENT_NAME = "movement_panel";
	const std::string& PANEL_CAMERA_NAME = "cam_panel";
}

BOOL LLBottomTray::postBuild()
{
	mBottomTrayContextMenu =  LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_bottomtray.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	gMenuHolder->addChild(mBottomTrayContextMenu);

	mNearbyChatBar = getChild<LLNearbyChatBar>("chat_bar");
	mToolbarStack = getChild<LLLayoutStack>("toolbar_stack");
	mMovementPanel = getChild<LLPanel>("movement_panel");
	mMovementButton = mMovementPanel->getChild<LLButton>("movement_btn");
	mGesturePanel = getChild<LLPanel>("gesture_panel");
	mCamPanel = getChild<LLPanel>("cam_panel");
	mCamButton = mCamPanel->getChild<LLButton>("camera_btn");
	mSnapshotPanel = getChild<LLPanel>("snapshot_panel");
	setRightMouseDownCallback(boost::bind(&LLBottomTray::showBottomTrayContextMenu,this, _2, _3,_4));

	if (mChicletPanel && mToolbarStack && mNearbyChatBar)
	{
		verifyChildControlsSizes();
	}

	return TRUE;
}

void LLBottomTray::verifyChildControlsSizes()
{
	LLRect rect = mChicletPanel->getRect();
	if (rect.getWidth() < mChicletPanel->getMinWidth())
	{
		mChicletPanel->reshape(mChicletPanel->getMinWidth(), rect.getHeight());
	}

	rect = mNearbyChatBar->getRect();
	if (rect.getWidth() < mNearbyChatBar->getMinWidth())
	{
		mNearbyChatBar->reshape(mNearbyChatBar->getMinWidth(), rect.getHeight());
	}
	else if (rect.getWidth() > mNearbyChatBar->getMaxWidth())
	{
		rect.setLeftTopAndSize(rect.mLeft, rect.mTop, mNearbyChatBar->getMaxWidth(), rect.getHeight());
		mNearbyChatBar->reshape(mNearbyChatBar->getMaxWidth(), rect.getHeight());
		mNearbyChatBar->setRect(rect);
	}
}

void LLBottomTray::reshape(S32 width, S32 height, BOOL called_from_parent)
{

	if (mChicletPanel && mToolbarStack && mNearbyChatBar)
	{
#ifdef __FEATURE_EXT_991__
		BOOL shrink = width < getRect().getWidth();
		const S32 MIN_RENDERED_CHARS = 3;
#endif

		verifyChildControlsSizes();
		updateResizeState(width, height);

		switch (mResizeState)
		{
		case STATE_CHICLET_PANEL:
			mToolbarStack->updatePanelAutoResize(PANEL_CHICLET_NAME, TRUE);

			mToolbarStack->updatePanelAutoResize(PANEL_CHATBAR_NAME, FALSE);
			mToolbarStack->updatePanelAutoResize(PANEL_CAMERA_NAME, FALSE);
			mToolbarStack->updatePanelAutoResize(PANEL_MOVEMENT_NAME, FALSE);

			break;
		case STATE_CHATBAR_INPUT:
			mToolbarStack->updatePanelAutoResize(PANEL_CHATBAR_NAME, TRUE);

			mToolbarStack->updatePanelAutoResize(PANEL_CHICLET_NAME, FALSE);
			mToolbarStack->updatePanelAutoResize(PANEL_CAMERA_NAME, FALSE);
			mToolbarStack->updatePanelAutoResize(PANEL_MOVEMENT_NAME, FALSE);

			break;

#ifdef __FEATURE_EXT_991__

		case STATE_BUTTONS:
			mToolbarStack->updatePanelAutoResize(PANEL_CAMERA_NAME, TRUE);
			mToolbarStack->updatePanelAutoResize(PANEL_MOVEMENT_NAME, TRUE);

			mToolbarStack->updatePanelAutoResize(PANEL_CHICLET_NAME, FALSE);
			mToolbarStack->updatePanelAutoResize(PANEL_CHATBAR_NAME, FALSE);

			if (shrink)
			{

				if (mSnapshotPanel->getVisible())
				{
					showSnapshotButton(FALSE);
				}

				if (mCamPanel->getVisible() && mCamButton->getLastDrawCharsCount() < MIN_RENDERED_CHARS)
				{
					showCameraButton(FALSE);
				}

				if (mMovementPanel->getVisible() && mMovementButton->getLastDrawCharsCount() < MIN_RENDERED_CHARS)
				{
					showMoveButton(FALSE);
				}

			}
			else
			{
				showMoveButton(TRUE);
				mMovementPanel->draw();

				if (mMovementButton->getLastDrawCharsCount() >= MIN_RENDERED_CHARS)
				{
					showMoveButton(TRUE);
				}
				else
				{
					showMoveButton(FALSE);
				}
			}
			break;
#endif

		default:
			break;
		}
	}

	LLPanel::reshape(width, height, called_from_parent);
}

void LLBottomTray::updateResizeState(S32 width, S32 height)
{
	mResizeState = STATE_BUTTONS;

	const S32 chiclet_panel_width = mChicletPanel->getRect().getWidth();
	const S32 chiclet_panel_min_width = mChicletPanel->getMinWidth();

	const S32 chatbar_panel_width = mNearbyChatBar->getRect().getWidth();
	const S32 chatbar_panel_min_width = mNearbyChatBar->getMinWidth();
	const S32 chatbar_panel_max_width = mNearbyChatBar->getMaxWidth();

	// bottom tray is narrowed
	if (width < getRect().getWidth())
	{
		if (chiclet_panel_width > chiclet_panel_min_width)
		{
			mResizeState = STATE_CHICLET_PANEL;
		}
		else if (chatbar_panel_width > chatbar_panel_min_width)
		{
			mResizeState = STATE_CHATBAR_INPUT;
		}
		else
		{
			mResizeState = STATE_BUTTONS;
		}
	}
	// bottom tray is widen
	else
	{
#ifdef __FEATURE_EXT_991__
		if (!mMovementPanel->getVisible())
		{
			mResizeState = STATE_BUTTONS;
		}
		else
#endif
		if (chatbar_panel_width < chatbar_panel_max_width)
		{
			mResizeState = STATE_CHATBAR_INPUT;
		}
		else
		{
			mResizeState = STATE_CHICLET_PANEL;
		}
	}


	// TODO: finish implementation
}
