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
,	mMovementButton(NULL)
// Add more members

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

	initStateProcessedObjectMap();

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
	const std::string& PANEL_CHICLET_NAME	= "chiclet_list_panel";
	const std::string& PANEL_CHATBAR_NAME	= "chat_bar";
	const std::string& PANEL_MOVEMENT_NAME	= "movement_panel";
	const std::string& PANEL_CAMERA_NAME	= "cam_panel";
	const std::string& PANEL_GESTURE_NAME	= "gesture_panel";
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

void LLBottomTray::log(LLView* panel, const std::string& descr)
{
	if (NULL == panel) return;
	LLView* layout = panel->getParent();
	lldebugs << descr << ": "
		<< "panel: " << panel->getName()
		<< ", rect: " << panel->getRect()
 
 
		<< "layout: " << layout->getName()
		<< ", rect: " << layout->getRect()
		<< llendl
		; 
}

void LLBottomTray::verifyChildControlsSizes()
{
	LLRect rect = mChicletPanel->getRect();
	/*
	if (rect.getWidth() < mChicletPanel->getMinWidth())
	{
		llwarns << "QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ: chiclet panel less then min" << llendl;
		mChicletPanel->reshape(mChicletPanel->getMinWidth(), rect.getHeight());
	}
*/
	rect = mNearbyChatBar->getRect();
/*
	if (rect.getWidth() < mNearbyChatBar->getMinWidth())
	{
		llwarns << "WWWWWWWWWWWWWWWWWWWWWWWWWWWWW: near chat panel less then min" << llendl;
		mNearbyChatBar->reshape(mNearbyChatBar->getMinWidth(), rect.getHeight());
	}
	else 
*/
		if (rect.getWidth() > mNearbyChatBar->getMaxWidth())
	{
		llerrs << "WWWWWWWWWWWWWWWWWWWWWWWWWWWWW: near chat panel more then max width" << llendl;

		rect.setLeftTopAndSize(rect.mLeft, rect.mTop, mNearbyChatBar->getMaxWidth(), rect.getHeight());
		mNearbyChatBar->reshape(mNearbyChatBar->getMaxWidth(), rect.getHeight());
		mNearbyChatBar->setRect(rect);
	}
}
#define __FEATURE_EXT_991
void LLBottomTray::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	lldebugs << "****************************************" << llendl;

	S32 current_width = getRect().getWidth();
	lldebugs << "Reshaping: " 
		<< ", width: " << width
		<< ", height: " << height
		<< ", called_from_parent: " << called_from_parent
		<< ", cur width: " << current_width
		<< ", cur height: " << getRect().getHeight()
		<< llendl;

	if (mNearbyChatBar)			log(mNearbyChatBar, "before");
	if (mChicletPanel)			log(mChicletPanel, "before");

	if (mChicletPanel && mToolbarStack && mNearbyChatBar)
	{
		mToolbarStack->updatePanelAutoResize(PANEL_CHICLET_NAME, TRUE);
 		verifyChildControlsSizes();
 		updateResizeState(width, current_width);
	}

	LLPanel::reshape(width, height, called_from_parent);


	if (mNearbyChatBar)			log(mNearbyChatBar, "after");
	if (mChicletPanel)			log(mChicletPanel, "after");
}

void LLBottomTray::updateResizeState(S32 new_width, S32 cur_width)
{
	mResizeState = RS_NORESIZE;
	MASK compensative_view_item_mask = RS_CHATBAR_INPUT;
	LLPanel* compansative_view = mNearbyChatBar;

	S32 delta_width = new_width - cur_width;
//	if (delta_width == 0) return;
	bool shrink = new_width < cur_width;

	const S32 chiclet_panel_width = mChicletPanel->getParent()->getRect().getWidth();
	const S32 chiclet_panel_min_width = mChicletPanel->getMinWidth();

	const S32 chatbar_panel_width = mNearbyChatBar->getRect().getWidth();
	const S32 chatbar_panel_min_width = mNearbyChatBar->getMinWidth();
	const S32 chatbar_panel_max_width = mNearbyChatBar->getMaxWidth();

	lldebugs << "chatbar_panel_width: " << chatbar_panel_width
		<< ", chatbar_panel_min_width: " << chatbar_panel_min_width
		<< ", chatbar_panel_max_width: " << chatbar_panel_max_width
		<< ", chiclet_panel_width: " << chiclet_panel_width
		<< ", chiclet_panel_min_width: " << chiclet_panel_min_width
		<< llendl;

	bool still_should_be_processed = true;
	// bottom tray is narrowed
	if (shrink)
	{
		S32 compensative_delta_width = 0;
		if (chiclet_panel_width > chiclet_panel_min_width)
		{
			// we have some space to decrease chiclet panel
			S32 panel_delta_min = chiclet_panel_width - chiclet_panel_min_width;
			mResizeState |= RS_CHICLET_PANEL;

			S32 delta_panel = llmin(-delta_width, panel_delta_min);

			lldebugs << "delta_width: " << delta_width
				<< ", panel_delta_min: " << panel_delta_min
				<< ", delta_panel: " << delta_panel
				<< llendl;

			// is chiclet panel width enough to process resizing?
			delta_width += panel_delta_min;

			still_should_be_processed = delta_width < 0;

			mChicletPanel->getParent()->reshape(mChicletPanel->getParent()->getRect().getWidth() - delta_panel, mChicletPanel->getParent()->getRect().getHeight());
			log(mChicletPanel, "after processing panel decreasing via chiclet panel");

			lldebugs << "RS_CHICLET_PANEL" 
				<< ", delta_width: " << delta_width
				<< llendl;
		}
		
		if (still_should_be_processed && chatbar_panel_width > chatbar_panel_min_width)
		{
			// we have some space to decrease chatbar panel
			S32 panel_delta_min = chatbar_panel_width - chatbar_panel_min_width;
			mResizeState |= RS_CHATBAR_INPUT;

			S32 delta_panel = llmin(-delta_width, panel_delta_min);

			// is chatbar panel width enough to process resizing?
			delta_width += panel_delta_min;
			

			still_should_be_processed = delta_width < 0;

			mNearbyChatBar->reshape(mNearbyChatBar->getRect().getWidth() - delta_panel, mNearbyChatBar->getRect().getHeight());

			lldebugs << "RS_CHATBAR_INPUT"
				<< ", delta_panel: " << delta_panel
				<< ", delta_width: " << delta_width
				<< llendl;

			log(mChicletPanel, "after nearby was processed");

		}
		if (still_should_be_processed)
		{
			mResizeState |= compensative_view_item_mask;

			if (mSnapshotPanel->getVisible())
			{
				mResizeState |= RS_BUTTON_SNAPSHOT;
				delta_width += mSnapshotPanel->getRect().getWidth();

				if (delta_width > 0)
				{
					compensative_delta_width += delta_width;
				}
				lldebugs << "RS_BUTTON_SNAPSHOT" 
					<< ", compensative_delta_width: " << compensative_delta_width
					<< ", delta_width: " << delta_width
					<< llendl;
				showSnapshotButton(false);
			}

			if (delta_width < 0 && mCamPanel->getVisible())
			{
				mResizeState |= RS_BUTTON_CAMERA;
				delta_width += mCamPanel->getRect().getWidth();
				if (delta_width > 0)
				{
					compensative_delta_width += delta_width;
				}
				lldebugs << "RS_BUTTON_CAMERA"
					<< ", compensative_delta_width: " << compensative_delta_width
					<< ", delta_width: " << delta_width
					<< llendl;
				showCameraButton(false);
			}

			if (delta_width < 0 && mMovementPanel->getVisible())
			{
				mResizeState |= RS_BUTTON_MOVEMENT;
				delta_width += mMovementPanel->getRect().getWidth();
				if (delta_width > 0)
				{
					compensative_delta_width += delta_width;
				}
				lldebugs << "RS_BUTTON_MOVEMENT"
					<< ", compensative_delta_width: " << compensative_delta_width
					<< ", delta_width: " << delta_width
					<< llendl;
				showMoveButton(false);
			}

			if (delta_width < 0 && mGesturePanel->getVisible())
			{
				mResizeState |= RS_BUTTON_GESTURES;
				delta_width += mGesturePanel->getRect().getWidth();
				if (delta_width > 0)
				{
					compensative_delta_width += delta_width;
				}
				lldebugs << "RS_BUTTON_GESTURES"
					<< ", compensative_delta_width: " << compensative_delta_width
					<< ", delta_width: " << delta_width
					<< llendl;
				showGestureButton(false);
			}

			if (delta_width < 0)
			{
				llwarns << "WARNING: there is no enough room for bottom tray, resizing still should be processed" << llendl;
			}

			if (compensative_delta_width != 0)
			{
				if (compansative_view)			log(compansative_view, "before applying compensative width: ");
				compansative_view->reshape(compansative_view->getRect().getWidth() + compensative_delta_width, compansative_view->getRect().getHeight() );
				if (compansative_view)			log(compansative_view, "after applying compensative width: ");
				lldebugs << compensative_delta_width << llendl;

			}
		}
	}
	// bottom tray is widen
	else
	{
		processWidthIncreased(delta_width);
	}

	lldebugs << "New resize state: " << mResizeState << llendl;
}

void LLBottomTray::processWidthDecreased(S32 delta_width)
{

}

void LLBottomTray::processWidthIncreased(S32 delta_width)
{
	const S32 chiclet_panel_width = mChicletPanel->getParent()->getRect().getWidth();
	const S32 chiclet_panel_min_width = mChicletPanel->getMinWidth();

	const S32 chatbar_panel_width = mNearbyChatBar->getRect().getWidth();
	const S32 chatbar_panel_min_width = mNearbyChatBar->getMinWidth();
	const S32 chatbar_panel_max_width = mNearbyChatBar->getMaxWidth();

	const S32 chatbar_available_shrink_width = chatbar_panel_width - chatbar_panel_min_width;
	const S32 available_width_chiclet = chiclet_panel_width - chiclet_panel_min_width;

	// how many room we have to show hidden buttons
	S32 available_width = delta_width + chatbar_available_shrink_width + available_width_chiclet;
	S32 buttons_required_width = 0; //How many room will take shown buttons

	if (available_width > 0)
	{
		lldebugs << "Trying to process: RS_BUTTON_GESTURES" << llendl;
		processShowButton(RS_BUTTON_GESTURES, &available_width, &buttons_required_width);
	}

	if (available_width > 0)
	{
		lldebugs << "Trying to process: RS_BUTTON_MOVEMENT" << llendl;
		processShowButton(RS_BUTTON_MOVEMENT, &available_width, &buttons_required_width);
	}

	if (available_width > 0)
	{
		lldebugs << "Trying to process: RS_BUTTON_CAMERA" << llendl;
		processShowButton(RS_BUTTON_CAMERA, &available_width, &buttons_required_width);
	}

	if (available_width > 0)
	{
		lldebugs << "Trying to process: RS_BUTTON_SNAPSHOT" << llendl;
		processShowButton(RS_BUTTON_SNAPSHOT, &available_width, &buttons_required_width);
	}

	// if we have to show some buttons but whidth increasing is not enough...
	if (buttons_required_width > 0 && delta_width < buttons_required_width)
	{
		// ... let's shrink nearby chat & chiclet panels
		S32 required_to_process_width = buttons_required_width;

		// 1. use delta width of resizing
		required_to_process_width -= delta_width;

		// 2. use width available via decreasing of nearby chat panel
		S32 chatbar_shrink_width = required_to_process_width;
		if (chatbar_available_shrink_width < chatbar_shrink_width)
		{
			chatbar_shrink_width = chatbar_available_shrink_width;
		}

		log(mNearbyChatBar, "increase width: before applying compensative width: ");
		mNearbyChatBar->reshape(mNearbyChatBar->getRect().getWidth() - chatbar_shrink_width, mNearbyChatBar->getRect().getHeight() );
		if (mNearbyChatBar)			log(mNearbyChatBar, "after applying compensative width: ");
		lldebugs << chatbar_shrink_width << llendl;

		// 3. use width available via decreasing of chiclet panel
		required_to_process_width -= chatbar_shrink_width;

		if (required_to_process_width > 0)
		{
			mChicletPanel->getParent()->reshape(mChicletPanel->getParent()->getRect().getWidth() - required_to_process_width, mChicletPanel->getParent()->getRect().getHeight());
			log(mChicletPanel, "after applying compensative width for chiclets: ");
			lldebugs << required_to_process_width << llendl;
		}

	}

	// shown buttons take some space, rest should be processed by nearby chatbar & chiclet panels
	delta_width -= buttons_required_width;

	// how many space can nearby chatbar take?
	S32 chatbar_panel_width_ = mNearbyChatBar->getRect().getWidth();
	if (delta_width > 0 && chatbar_panel_width_ < chatbar_panel_max_width)
	{
		mResizeState |= RS_CHATBAR_INPUT;
		S32 delta_panel_max = chatbar_panel_max_width - chatbar_panel_width_;
		S32 delta_panel = llmin(delta_width, delta_panel_max);
		delta_width -= delta_panel_max;
		mNearbyChatBar->reshape(chatbar_panel_width_ + delta_panel, mNearbyChatBar->getRect().getHeight());
	}
}

bool LLBottomTray::processShowButton(EResizeState shown_object_type, S32* available_width, S32* buttons_required_width)
{
	LLPanel* panel = mStateProcessedObjectMap[shown_object_type];
	if (NULL == panel)
	{
		lldebugs << "There is no object to process for state: " << shown_object_type << llendl;
		return false;
	}
	bool can_be_shown = canButtonBeShown(panel);
	if (can_be_shown)
	{
		//validate if we have enough room to show this button
		const S32 required_width = panel->getRect().getWidth();
		can_be_shown = *available_width >= required_width;
		if (can_be_shown)
		{
			*available_width -= required_width;
			*buttons_required_width += required_width;

			switch (shown_object_type)
			{
			case RS_BUTTON_GESTURES:	showGestureButton(true);				break;
			case RS_BUTTON_MOVEMENT:	showMoveButton(true);					break;
			case RS_BUTTON_CAMERA:		showCameraButton(true);					break;
			case RS_BUTTON_SNAPSHOT:	showSnapshotButton(true);				break;
			default:
				llwarns << "Unexpected type of button to be shown: " << shown_object_type << llendl;
			}

			lldebugs << "processing object type: " << shown_object_type
				<< ", buttons_required_width: " << buttons_required_width
				<< llendl;
		}
	}
	return can_be_shown;
}

bool LLBottomTray::canButtonBeShown(LLPanel* panel) const
{
	bool can_be_shown = !panel->getVisible();
	if (can_be_shown)
	{
		// *TODO: mantipov: synchronize with situation when button was hidden via context menu;
	}
	return can_be_shown;
}

void LLBottomTray::initStateProcessedObjectMap()
{
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_GESTURES, mGesturePanel));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_MOVEMENT, mMovementPanel));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_CAMERA, mCamPanel));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_SNAPSHOT, mSnapshotPanel));
}
//EOF
