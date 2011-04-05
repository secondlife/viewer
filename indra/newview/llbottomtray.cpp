/** 
 * @file llbottomtray.cpp
 * @brief LLBottomTray class implementation
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

#include "llviewerprecompiledheaders.h" // must be first include

#define LLBOTTOMTRAY_CPP
#include "llbottomtray.h"

// library includes
#include "llfloaterreg.h"
#include "llflyoutbutton.h"
#include "lllayoutstack.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "lltexteditor.h"

// newview includes
#include "llagentcamera.h"
#include "llchiclet.h"
#include "llfloatercamera.h"
#include "llhints.h"
#include "llimfloater.h" // for LLIMFloater
#include "llnearbychatbar.h"
#include "llspeakbutton.h"
#include "llsplitbutton.h"
#include "llsyswellwindow.h"
#include "lltoolmgr.h"
#include "llviewerparcelmgr.h"

#include "llviewerwindow.h"
#include "llsdserialize.h"

// Distance from mouse down on which drag'n'drop should be started.
#define DRAG_START_DISTANCE 3

static const std::string SORTING_DATA_FILE_NAME = "bottomtray_buttons_order.xml";

LLDefaultChildRegistry::Register<LLBottomtrayButton> bottomtray_button("bottomtray_button");

// LLBottomtrayButton methods

// virtual
BOOL LLBottomtrayButton::handleHover(S32 x, S32 y, MASK mask)
{
	if (mCanDrag)
	{
		// pass hover to bottomtray
		S32 screenX, screenY;
		localPointToScreen(x, y, &screenX, &screenY);
		LLBottomTray::getInstance()->onDraggableButtonHover(screenX, screenY);

		return TRUE;
	}
	else
	{
		return LLButton::handleHover(x, y, mask);
	}
}
//virtual
BOOL LLBottomtrayButton::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (mCanDrag)
	{
	S32 screenX, screenY;
	localPointToScreen(x, y, &screenX, &screenY);
	// pass mouse up to bottomtray
	LLBottomTray::getInstance()->onDraggableButtonMouseUp(this, screenX, screenY);
	}
	return LLButton::handleMouseUp(x, y, mask);
}
//virtual
BOOL LLBottomtrayButton::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mCanDrag)
	{
	S32 screenX, screenY;
	localPointToScreen(x, y, &screenX, &screenY);
	// pass mouse up to bottomtray
	LLBottomTray::getInstance()->onDraggableButtonMouseDown(this, screenX, screenY);
	}
	return LLButton::handleMouseDown(x, y, mask);
}

static void update_build_button_enable_state()
{
	bool can_edit = LLToolMgr::getInstance()->canEdit();

	LLBottomTray::getInstance()->getChildView("build_btn")->setEnabled(can_edit);
}

// Build time optimization, generate extern template once in .cpp file
template class LLBottomTray* LLSingleton<class LLBottomTray>::getInstance();

namespace
{
	const std::string& PANEL_CHICLET_NAME	= "chiclet_list_panel";

	S32 get_panel_min_width(LLLayoutStack* stack, LLView* panel)
	{
		S32 minimal_width = 0;
		llassert(stack);
		if ( stack && panel && panel->getVisible() )
		{
			stack->getPanelMinSize(panel->getName(), &minimal_width);
		}
		return minimal_width;
	}

	S32 get_panel_max_width(LLLayoutStack* stack, LLPanel* panel)
	{
		S32 max_width = 0;
		llassert(stack);
		if ( stack && panel && panel->getVisible() )
		{
			stack->getPanelMaxSize(panel->getName(), &max_width);
		}
		return max_width;
	}

	S32 get_curr_width(LLUICtrl* ctrl)
	{
		S32 cur_width = 0;
		if ( ctrl && ctrl->getVisible() )
		{
			cur_width = ctrl->getRect().getWidth();
		}
		return cur_width;
	}
}

class LLBottomTrayLite
	: public LLPanel
{
public:
	LLBottomTrayLite()
		: mNearbyChatBar(NULL),
		mChatBarContainer(NULL),
		  mGesturePanel(NULL)
	{
		mFactoryMap["chat_bar"] = LLCallbackMap(LLBottomTray::createNearbyChatBar, NULL);
		buildFromFile("panel_bottomtray_lite.xml");
	}

	BOOL postBuild()
	{
		mNearbyChatBar = findChild<LLNearbyChatBar>("chat_bar");
		mChatBarContainer = getChild<LLLayoutPanel>("chat_bar_layout_panel");
		mGesturePanel = getChild<LLPanel>("gesture_panel");

		// Hide "show_nearby_chat" button 
		if (mNearbyChatBar)
		{
			LLLineEditor* chat_box = mNearbyChatBar->getChatBox();
			LLUICtrl* show_btn = mNearbyChatBar->getChild<LLUICtrl>("show_nearby_chat");
			S32 delta_width = show_btn->getRect().getWidth();
			show_btn->setVisible(FALSE);
			chat_box->reshape(chat_box->getRect().getWidth() + delta_width, chat_box->getRect().getHeight());
		}
		return TRUE;
	}

	void onFocusLost()
	{
		if (gAgentCamera.cameraMouselook())
		{
			LLBottomTray::getInstance()->setVisible(FALSE);
		}
	}

	LLNearbyChatBar*	mNearbyChatBar;
	LLLayoutPanel*		mChatBarContainer;
	LLPanel*			mGesturePanel;
};

LLBottomTray::LLBottomTray(const LLSD&)
:	mChicletPanel(NULL),
	mSpeakPanel(NULL),
	mSpeakBtn(NULL),
	mNearbyChatBar(NULL),
	mChatBarContainer(NULL),
	mNearbyCharResizeHandlePanel(NULL),
	mToolbarStack(NULL),
	mMovementButton(NULL),
	mResizeState(RS_NORESIZE),
	mBottomTrayContextMenu(NULL),
	mCamButton(NULL),
	mBottomTrayLite(NULL),
	mIsInLiteMode(false),
	mDragStarted(false),
	mDraggedItem(NULL),
	mLandingTab(NULL),
	mCheckForDrag(false)
{
	// Firstly add ourself to IMSession observers, so we catch session events
	// before chiclets do that.
	LLIMMgr::getInstance()->addSessionObserver(this);

	mFactoryMap["chat_bar"] = LLCallbackMap(LLBottomTray::createNearbyChatBar, NULL);

	buildFromFile("panel_bottomtray.xml");

	LLUICtrl::CommitCallbackRegistry::defaultRegistrar().add("CameraPresets.ChangeView", boost::bind(&LLFloaterCamera::onClickCameraItem, _2));

	//this is to fix a crash that occurs because LLBottomTray is a singleton
	//and thus is deleted at the end of the viewers lifetime, but to be cleanly
	//destroyed LLBottomTray requires some subsystems that are long gone
	//LLUI::getRootView()->addChild(this);

	{
		mBottomTrayLite = new LLBottomTrayLite();
		mBottomTrayLite->setFollowsAll();
		mBottomTrayLite->setVisible(FALSE);
	}

	mImageDragIndication = LLUI::getUIImage(getString("DragIndicationImageName"));
	mDesiredNearbyChatWidth = mNearbyChatBar ? mNearbyChatBar->getRect().getWidth() : 0;
}

LLBottomTray::~LLBottomTray()
{
	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}

	if (mNearbyChatBar)
	{
		// store custom width of chatbar panel.
		S32 custom_width = mChatBarContainer->getRect().getWidth();
		gSavedSettings.setS32("ChatBarCustomWidth", custom_width);
	}

	// emulate previous floater behavior to be hidden on startup.
	// override effect of save_visibility=true.
	// this attribute is necessary to button.initial_callback=Button.SetFloaterToggle works properly:
	//		i.g when floater changes its visibility - button changes its toggle state.
	getChild<LLUICtrl>("build_btn")->setControlValue(false);
	getChild<LLUICtrl>("search_btn")->setControlValue(false);
	getChild<LLUICtrl>("world_map_btn")->setControlValue(false);
}

// *TODO Vadim: why void* ?
void* LLBottomTray::createNearbyChatBar(void* userdata)
{
	return new LLNearbyChatBar();
}

LLNearbyChatBar* LLBottomTray::getNearbyChatBar()
{
	return mIsInLiteMode ? mBottomTrayLite->mNearbyChatBar : mNearbyChatBar;
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
	if (!getChicletPanel()) return;

	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
	if (!session) return;

	// no need to spawn chiclets for participants in P2P calls called through Avaline
	if (session->isP2P() && session->isOtherParticipantAvaline()) return;

	if (getChicletPanel()->findChiclet<LLChiclet>(session_id)) return;

	LLIMChiclet* chiclet = createIMChiclet(session_id);
	if(chiclet)
	{
		chiclet->setIMSessionName(name);
		chiclet->setOtherParticipantId(other_participant_id);
		
		LLIMFloater::onIMChicletCreated(session_id);

	}
	else
	{
		llerrs << "Could not create chiclet" << llendl;
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

S32 LLBottomTray::getTotalUnreadIMCount()
{
	return getChicletPanel()->getTotalUnreadIMCount();
}

// virtual
void LLBottomTray::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	// Time it takes to connect to voice channel might be pretty long,
	// so don't expect user login or STATUS_VOICE_ENABLED to be followed by STATUS_JOINED.
	BOOL enable = FALSE;

	switch (status)
	{
	// Do not add STATUS_VOICE_ENABLED because voice chat is 
	// inactive until STATUS_JOINED
	case STATUS_JOINED:
		enable = TRUE;
		break;
	default:
		enable = FALSE;
		break;
	}

	// We have to enable/disable right and left parts of speak button separately (EXT-4648)
	mSpeakBtn->setSpeakBtnEnabled(enable);
	// skipped to avoid button blinking
	if (status != STATUS_JOINING && status!= STATUS_LEFT_CHANNEL)
	{
		mSpeakBtn->setFlyoutBtnEnabled(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking());
	}
}

void LLBottomTray::onMouselookModeOut()
{
	mIsInLiteMode = false;
	mBottomTrayLite->setVisible(FALSE);
	mNearbyChatBar->getChatBox()->setText(mBottomTrayLite->mNearbyChatBar->getChatBox()->getText());
	setVisible(TRUE);
}

void LLBottomTray::onMouselookModeIn()
{
	setVisible(FALSE);

	// Attach the lite bottom tray
	if (getParent() && mBottomTrayLite->getParent() != getParent())
		getParent()->addChild(mBottomTrayLite);

	mBottomTrayLite->setShape(getLocalRect());
	mBottomTrayLite->mNearbyChatBar->getChatBox()->setText(mNearbyChatBar->getChatBox()->getText());
	mBottomTrayLite->mGesturePanel->setVisible(gSavedSettings.getBOOL("ShowGestureButton"));

	mIsInLiteMode = true;
}

//virtual
// setVisible used instead of onVisibilityChange, since LLAgent calls it on entering/leaving mouselook mode.
// If bottom tray is already visible in mouselook mode, then onVisibilityChange will not be called from setVisible(true),
void LLBottomTray::setVisible(BOOL visible)
{
	if (mIsInLiteMode)
	{
		mBottomTrayLite->setVisible(visible);
	}
	else 
	{
		LLPanel::setVisible(visible);
	}
	if(visible)
		gFloaterView->setSnapOffsetBottom(getRect().getHeight());
	else
		gFloaterView->setSnapOffsetBottom(0);
}

S32 LLBottomTray::notifyParent(const LLSD& info)
{
	if(info.has("well_empty")) // implementation of EXT-3397
	{
		const std::string chiclet_name = info["well_name"];

		// only "im_well" or "notification_well" names are expected.
		// They are set in panel_bottomtray.xml in <chiclet_im_well> & <chiclet_notification>
		llassert("im_well" == chiclet_name || "notification_well" == chiclet_name);

		BOOL should_be_visible = !info["well_empty"];
		showWellButton("im_well" == chiclet_name ? RS_IM_WELL : RS_NOTIFICATION_WELL, should_be_visible);
		return 1;
	}

	if (info.has("action") && info["action"] == "resize")
	{
		const std::string& name = info["view_name"];

		// expected only resize of nearby chatbar
		if (mChatBarContainer->getName() != name) return LLPanel::notifyParent(info);

		const S32 new_width = info["new_width"];

		processChatbarCustomization(new_width);

		return 2;
	}
	return LLPanel::notifyParent(info);
}

void LLBottomTray::showBottomTrayContextMenu(S32 x, S32 y, MASK mask)
{
	// We should show BottomTrayContextMenu in last  turn
	if (mBottomTrayContextMenu && !LLMenuGL::sMenuContainer->getVisibleMenu())
	{
		    //there are no other context menu (IM chiclet etc ), so we can show BottomTrayContextMenu

		    updateContextMenu(x, y, mask);
			mBottomTrayContextMenu->buildDrawLabels();
			mBottomTrayContextMenu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(this, mBottomTrayContextMenu, x, y);
		
	}
}

void LLBottomTray::updateContextMenu(S32 x, S32 y, MASK mask)
{
	LLUICtrl* edit_box = mNearbyChatBar->getChild<LLUICtrl>("chat_box");

	S32 local_x = x - mChatBarContainer->getRect().mLeft - edit_box->getRect().mLeft;
	S32 local_y = y - mChatBarContainer->getRect().mBottom - edit_box->getRect().mBottom;

	bool in_edit_box = edit_box->pointInView(local_x, local_y);

	mBottomTrayContextMenu->setItemVisible("Separator", in_edit_box);
	mBottomTrayContextMenu->setItemVisible("NearbyChatBar_Cut", in_edit_box);
	mBottomTrayContextMenu->setItemVisible("NearbyChatBar_Copy", in_edit_box);
	mBottomTrayContextMenu->setItemVisible("NearbyChatBar_Paste", in_edit_box);
	mBottomTrayContextMenu->setItemVisible("NearbyChatBar_Delete", in_edit_box);
	mBottomTrayContextMenu->setItemVisible("NearbyChatBar_Select_All", in_edit_box);
}

void LLBottomTray::showGestureButton(BOOL visible)
{
	setTrayButtonVisibleIfPossible(RS_BUTTON_GESTURES, visible);
}

void LLBottomTray::showMoveButton(BOOL visible)
{
	setTrayButtonVisibleIfPossible(RS_BUTTON_MOVEMENT, visible);
}

void LLBottomTray::showCameraButton(BOOL visible)
{
	setTrayButtonVisibleIfPossible(RS_BUTTON_CAMERA, visible);
}

void LLBottomTray::showSnapshotButton(BOOL visible)
{
	setTrayButtonVisibleIfPossible(RS_BUTTON_SNAPSHOT, visible);
}

void LLBottomTray::showSpeakButton(bool visible)
{
	// Show/hide the button
	setTrayButtonVisible(RS_BUTTON_SPEAK, visible);

	// and adjust other panels according to the occupied/freed space.
	const S32 panel_width = mSpeakPanel->getRect().getWidth();
	if (visible)
	{
		processWidthDecreased(-panel_width);
	}
	else
	{
		processWidthIncreased(panel_width);
	}
}

void LLBottomTray::toggleMovementControls()
{
	if (mMovementButton)
		mMovementButton->onCommit();
}

void LLBottomTray::toggleCameraControls()
{
	if (mCamButton)
		mCamButton->onCommit();
}

BOOL LLBottomTray::postBuild()
{
	LLHints::registerHintTarget("bottom_tray", LLView::getHandle());
	LLHints::registerHintTarget("dest_guide_btn", getChild<LLUICtrl>("destination_btn")->getHandle());
	LLHints::registerHintTarget("avatar_picker_btn", getChild<LLUICtrl>("avatar_btn")->getHandle());

	LLUICtrl::CommitCallbackRegistry::currentRegistrar().add("NearbyChatBar.Action", boost::bind(&LLBottomTray::onContextMenuItemClicked, this, _2));
	LLUICtrl::EnableCallbackRegistry::currentRegistrar().add("NearbyChatBar.EnableMenuItem", boost::bind(&LLBottomTray::onContextMenuItemEnabled, this, _2));

	mBottomTrayContextMenu =  LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_bottomtray.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	gMenuHolder->addChild(mBottomTrayContextMenu);

	mNearbyChatBar = findChild<LLNearbyChatBar>("chat_bar");
	LLHints::registerHintTarget("chat_bar", mNearbyChatBar->LLView::getHandle());

	mChatBarContainer = getChild<LLLayoutPanel>("chat_bar_layout_panel");
	mNearbyCharResizeHandlePanel = getChild<LLPanel>("chat_bar_resize_handle_panel");

	mToolbarStack = getChild<LLLayoutStack>("toolbar_stack");
	mMovementButton = getChild<LLButton>("movement_btn");
	LLHints::registerHintTarget("move_btn", mMovementButton->getHandle());
	mCamButton = getChild<LLButton>("camera_btn");
	setRightMouseDownCallback(boost::bind(&LLBottomTray::showBottomTrayContextMenu,this, _2, _3,_4));

	mSpeakPanel = getChild<LLPanel>("speak_panel");
	mSpeakBtn = getChild<LLSpeakButton>("talk");

	// Both parts of speak button should be initially disabled because
	// it takes some time between logging in to world and connecting to voice channel.
	mSpeakBtn->setSpeakBtnEnabled(false);
	mSpeakBtn->setFlyoutBtnEnabled(false);

	// Localization tool doesn't understand custom buttons like <talk_button>
	mSpeakBtn->setSpeakToolTip( getString("SpeakBtnToolTip") );
	mSpeakBtn->setShowToolTip( getString("VoiceControlBtnToolTip") );

	// Registering Chat Bar to receive Voice client status change notifications.
	LLVoiceClient::getInstance()->addObserver(this);

	mNearbyChatBar->getChatBox()->setContextMenu(NULL);

	mChicletPanel = getChild<LLChicletPanel>("chiclet_list");

	initResizeStateContainers();

	setButtonsControlsAndListeners();

	initButtonsVisibility();

	// update wells visibility:
	showWellButton(RS_IM_WELL, !LLIMWellWindow::getInstance()->isWindowEmpty());
	showWellButton(RS_NOTIFICATION_WELL, !LLNotificationWellWindow::getInstance()->isWindowEmpty());

	loadButtonsOrder();

	LLViewerParcelMgr::getInstance()->addAgentParcelChangedCallback(boost::bind(&update_build_button_enable_state));

	return TRUE;
}

//Drag-n-drop

void LLBottomTray::onDraggableButtonMouseDown(LLUICtrl* ctrl, S32 x, S32 y)
{
	if (ctrl == NULL) return;
	LLView* parent_view = ctrl->getParent();
	if(parent_view != NULL)
	{
		// we actually drag'n'drop panel (not button) in code, so have to find a parent
		// of button which called this method on mouse down.
		LLPanel* parent = dynamic_cast<LLPanel*>(parent_view);
		// It may happen that we clicked not usual button, but button inside widget(speak, gesture)
		// so we'll need to get a level higher to reach layout panel as a parent.
		if(parent == NULL) parent = dynamic_cast<LLPanel*>(parent_view->getParent());
		if (parent && parent->getVisible())
		{
			mDraggedItem = parent;
			mCheckForDrag = true;
			mStartX = x;
			mStartY = y;
		}
	}
}

LLPanel* LLBottomTray::findChildPanelByLocalCoords(S32 x, S32 y)
{
	LLPanel* ctrl = 0;
	S32 screenX, screenY;
	const child_list_t* list = mToolbarStack->getChildList();

	localPointToScreen(x, y, &screenX, &screenY);

	// look for a child panel which contains the point (screenX, screenY) in it's rectangle
	for (child_list_const_iter_t i = list->begin(); i != list->end(); ++i)
	{
		LLRect rect;
		localRectToScreen((*i)->getRect(), &rect);

		if (rect.pointInRect(screenX, screenY))
		{
			ctrl = dynamic_cast<LLPanel*>(*i);
			break;
		}
	}

	return ctrl;
}

void LLBottomTray::onDraggableButtonHover(S32 x, S32 y)
{
	// if mouse down on draggable item was done, check whether we should start DnD
	if (mCheckForDrag)
	{
		// Start drag'n'drop if mouse cursor was dragged away frome mouse down location enough
		if(sqrt((float)((mStartX-x)*(mStartX-x)+(mStartY-y)*(mStartY-y))) > DRAG_START_DISTANCE)
		{
			mDragStarted = true;
			mCheckForDrag = false;
		}
	}
	if (mDragStarted)
	{
		// Check whether the cursor is over draggable area, find which panel it is and set is as
		// landing tab for drag'n'drop
		if(isCursorOverDraggableArea(x, y))
		{
			LLPanel* panel = findChildPanelByLocalCoords(x,y);
			if (panel && panel != mDraggedItem) mLandingTab = panel;
			gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROWDRAG);
		}
		else
		{
			gViewerWindow->getWindow()->setCursor(UI_CURSOR_NO);
		}
	}
	else
	{
		// Reset cursor in case you move your mouse from the drag handle to a button.
		getWindow()->setCursor(UI_CURSOR_ARROW);

	}
}

bool LLBottomTray::isCursorOverDraggableArea(S32 x, S32 y)
{
	// Draggable area lasts from the nearby chat input resize handle
	// to the chiclet area (exlusively).
	bool result = getRect().pointInRect(x, y);
	result = result && mNearbyCharResizeHandlePanel->calcScreenRect().mRight < x;
	result = result && mChicletPanel->calcScreenRect().mRight > x;
	return result;
}

void LLBottomTray::updateButtonsOrdersAfterDnD()
{
	// *TODO: change implementation of this method to support simplify it
	// (and according to future possible changes in the way button order is saved between sessions).
	state_object_map_t::const_iterator it = mStateProcessedObjectMap.begin();
	state_object_map_t::const_iterator it_end = mStateProcessedObjectMap.end();
	EResizeState dragged_state = RS_NORESIZE;
	EResizeState landing_state = RS_NORESIZE;
	bool landing_state_found = false;
	// Find states for dragged item and landing tab
	for (; it != it_end; ++it)
	{
		if (it->second == mDraggedItem)
		{
			dragged_state = it->first;
		}
		else if (it->second == mLandingTab)
		{
			landing_state = it->first;
			landing_state_found = true;
		}
	}

	if (dragged_state == RS_NORESIZE)
	{
		llwarns << "Cannot determine what button is being dragged" << llendl;
		llassert(dragged_state != RS_NORESIZE);
		return;
	}

	lldebugs << "Will place " << resizeStateToString(dragged_state)
		<< " before " << resizeStateToString(landing_state) << llendl;

	// Update order of buttons according to drag'n'drop
	mButtonsOrder.erase(std::find(mButtonsOrder.begin(), mButtonsOrder.end(), dragged_state));
	if (!landing_state_found && mLandingTab == getChild<LLPanel>(PANEL_CHICLET_NAME))
	{
		mButtonsOrder.push_back(dragged_state);
	}
	else
	{
		if (!landing_state_found) landing_state = RS_BUTTON_SPEAK; // just a random fallback
		mButtonsOrder.insert(std::find(mButtonsOrder.begin(), mButtonsOrder.end(), landing_state), dragged_state);
	}

	// Synchronize button process order with their order
	resize_state_vec_t::const_iterator it1 = mButtonsOrder.begin();
	const resize_state_vec_t::const_iterator it_end1 = mButtonsOrder.end();
	resize_state_vec_t::iterator it2 = mButtonsProcessOrder.begin();
	for (; it1 != it_end1; ++it1)
	{
		// Skip Speak because it is not in mButtonsProcessOrder(it's the reason why mButtonsOrder was introduced).
		// If any other draggable items will be added to bottomtray later, they should also be skipped here.
		if (*it1 != RS_BUTTON_SPEAK)
		{
			*it2 = *it1;
			++it2;
		}
	}

	saveButtonsOrder();
}

void LLBottomTray::saveButtonsOrder()
{
	std::string user_dir = gDirUtilp->getLindenUserDir();
	if (user_dir.empty()) return;
	
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SORTING_DATA_FILE_NAME);
	LLSD settings_llsd;
	int i = 0;
	const resize_state_vec_t::const_iterator it_end = mButtonsOrder.end();
	// we use numbers as keys for map which is saved in file and contains resize states as its values
	for (resize_state_vec_t::const_iterator it = mButtonsOrder.begin(); it != it_end; ++it, i++)
	{
		std::string str = llformat("%d", i);
		settings_llsd[str] = *it;		
	}
	llofstream file;
	file.open(filename);
	LLSDSerialize::toPrettyXML(settings_llsd, file);
}

void LLBottomTray::loadButtonsOrder()
{
	// load per-resident sorting information
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SORTING_DATA_FILE_NAME);

	LLSD settings_llsd;
	llifstream file;
	file.open(filename);
	if (!file.is_open()) return;
	
	LLSDSerialize::fromXML(settings_llsd, file);
	

	mButtonsOrder.clear();
	mButtonsProcessOrder.clear();
	int i = 0;
	// getting button order from file
	for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
		iter != settings_llsd.endMap(); ++iter, ++i)
	{
		std::string str = llformat("%d", i);
		EResizeState state = (EResizeState)settings_llsd[str].asInteger();
		mButtonsOrder.push_back(state);
		// RS_BUTTON_SPEAK is skipped, because it shouldn't be in mButtonsProcessOrder (it does not hide or shrink).
		if (state != RS_BUTTON_SPEAK)
		{
			mButtonsProcessOrder.push_back(state);
		}		
	}

	// There are other panels in layout stack order of which is not saved. Also, panels order of which is saved,
	// are already in layout stack but in wrong order. The most convenient way to place them is moving them 
	// to front one by one (because in this case we don't have to pass the panel before which we want to insert our
	// panel to movePanel()). So panels are moved in order from the end of mButtonsOrder vector(reverse iterator is used).
	const resize_state_vec_t::const_reverse_iterator it_end = mButtonsOrder.rend();
	// placing panels in layout stack according to button order which we loaded in previous for
	for (resize_state_vec_t::const_reverse_iterator it = mButtonsOrder.rbegin(); it != it_end; ++it, ++i)
	{
		LLPanel* panel_to_move = getButtonPanel(*it);
		mToolbarStack->movePanel(panel_to_move, NULL, true); // prepend 		
	}
	// Nearbychat is not stored in order settings file, but it must be the first of the panels, so moving it
	// (along with its drag handle) manually here.
	mToolbarStack->movePanel(getChild<LLLayoutPanel>("chat_bar_resize_handle_panel"), NULL, true);
	mToolbarStack->movePanel(mChatBarContainer, NULL, true);
}

void LLBottomTray::onDraggableButtonMouseUp(LLUICtrl* ctrl, S32 x, S32 y)
{
	//if mouse up happened over area where drop is possible, change order of buttons
	if (mLandingTab != NULL && mDraggedItem != NULL && mDragStarted)
	{
		if(isCursorOverDraggableArea(x, y))
		{
			// change order of panels in layout stack
			mToolbarStack->movePanel(mDraggedItem, (LLPanel*)mLandingTab);
			// change order of buttons in order vectors
			updateButtonsOrdersAfterDnD();
		}
	}
	gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROW);
	mDragStarted = false;
	mDraggedItem = NULL;
	mLandingTab = NULL;
	mCheckForDrag = false;
}

void LLBottomTray::draw()
{
	LLPanel::draw();
	if (mLandingTab)
	{
		static S32 w = mImageDragIndication->getWidth();
		static S32 h = mImageDragIndication->getHeight();
		LLRect rect = mLandingTab->calcScreenRect();
		mImageDragIndication->draw(rect.mLeft - w/2, rect.getHeight(), w, h);
	}
}

bool LLBottomTray::onContextMenuItemEnabled(const LLSD& userdata)
{
	std::string item = userdata.asString();
	LLLineEditor* edit_box = mNearbyChatBar->findChild<LLLineEditor>("chat_box");
	
	if (item == "can_cut")
	{
		return edit_box->canCut();
	}
	else if (item == "can_copy")
	{
		return edit_box->canCopy();
	}
	else if (item == "can_paste")
	{
		return edit_box->canPaste();
	}
	else if (item == "can_delete")
	{
		return edit_box->canDoDelete();
	}
	else if (item == "can_select_all")
	{
		return edit_box->canSelectAll() && (edit_box->getLength()>0);
	}
	return true;
}


void LLBottomTray::onContextMenuItemClicked(const LLSD& userdata)
{
	std::string item = userdata.asString();
	LLLineEditor* edit_box = mNearbyChatBar->findChild<LLLineEditor>("chat_box");

	if (item == "cut")
	{
		edit_box->cut();
	}
	else if (item == "copy")
	{
		edit_box->copy();
	}
	else if (item == "paste")
	{
		edit_box->paste();
		edit_box->setFocus(TRUE);
	}
	else if (item == "delete")
	{
		edit_box->doDelete();
	}
	else if (item == "select_all")
	{
		edit_box->selectAll();
	}
}

void LLBottomTray::log(LLView* panel, const std::string& descr)
{
	if (NULL == panel) return;
	LLView* layout = panel->getParent();
	lldebugs << descr << ": "
		<< "panel: " << panel->getName()
		<< ", rect: " << panel->getRect()
 
 
		<< " layout: " << layout->getName()
		<< ", rect: " << layout->getRect()
		<< llendl
		; 
}

void LLBottomTray::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	static S32 debug_calling_number = 0;
	lldebugs << "**************************************** " << ++debug_calling_number << llendl;

	S32 current_width = getRect().getWidth();
	S32 delta_width = width - current_width;
	lldebugs << "Reshaping: " 
		<< ", width: " << width
		<< ", cur width: " << current_width
		<< ", delta_width: " << delta_width
		<< ", called_from_parent: " << called_from_parent
		<< llendl;

	if (mNearbyChatBar)			log(mNearbyChatBar, "before");
	if (mChicletPanel)			log(mChicletPanel, "before");

	// Difference between bottom tray width required to fit its children and the actual width. (see EXT-991)
	// Positive value means that bottom tray is not wide enough.
	// Negative value means that there is free space.
	static S32 extra_shrink_width = 0;
	bool should_be_reshaped = true;

	if (mChicletPanel && mToolbarStack && mNearbyChatBar)
	{
		// Firstly, update layout stack to ensure we deal with correct panel sizes.
		{
			BOOL saved_anim = mToolbarStack->getAnimate();
			// Set chiclet panel to be autoresized by default.
			mToolbarStack->updatePanelAutoResize(PANEL_CHICLET_NAME, TRUE);
			// Disable animation to prevent layout updating in several frames.
			mToolbarStack->setAnimate(FALSE);
			// Force the updating of layout to reset panels collapse factor.
			mToolbarStack->updateLayout();
			// Restore animate state.
			mToolbarStack->setAnimate(saved_anim);
		}

		// bottom tray is narrowed
		if (delta_width < 0)
		{
			if (extra_shrink_width > 0) // not enough space
			{
				extra_shrink_width += llabs(delta_width);
				should_be_reshaped = false;
			}
			else
			{
				extra_shrink_width = processWidthDecreased(delta_width);

				// increase new width to extra_shrink_width value to not reshape less than bottom tray minimum
				width += extra_shrink_width;
			}
		}
		// bottom tray is widened
		else
		{
			if (extra_shrink_width > delta_width)
			{
				// Still not enough space.
				// Only subtract the delta from the required delta and don't reshape.
				extra_shrink_width -= delta_width;
				should_be_reshaped = false;
			}
			else 
			{
				if (extra_shrink_width > 0)
				{
					// If we have some extra shrink width let's reduce delta_width & width
					delta_width -= extra_shrink_width;
					width -= extra_shrink_width;
					extra_shrink_width = 0;
				}
				processWidthIncreased(delta_width);
			}
		}
	}

	if (should_be_reshaped)
	{
		lldebugs << "Reshape all children with width: " << width << llendl;
		LLPanel::reshape(width, height, called_from_parent);
	}

	if (mNearbyChatBar)			log(mNearbyChatBar, "after");
	if (mChicletPanel)			log(mChicletPanel, "after");


	// Restore width of the chatbar on first reshape.
	// we can not to do this from postBuild because reshape is called from parent view on startup
	// creation after it and reset width according to resize logic.
	static bool needs_restore_custom_state = true;
	if (mChatBarContainer && needs_restore_custom_state)
	{
		// restore custom width of chatbar panel.
		S32 new_width = gSavedSettings.getS32("ChatBarCustomWidth");
		if (new_width > 0)
		{
			mDesiredNearbyChatWidth = new_width;
			processChatbarCustomization(new_width);
			lldebugs << "Setting nearby chat bar width to " << new_width << " px" << llendl;
			mChatBarContainer->reshape(new_width, mChatBarContainer->getRect().getHeight());
		}
		needs_restore_custom_state = false;
	}

}

S32 LLBottomTray::processWidthDecreased(S32 delta_width)
{
	bool still_should_be_processed = true;

	const S32 chiclet_panel_shrink_headrom = getChicletPanelShrinkHeadroom();

	// There are four steps of processing width decrease. If in one of them required width was reached,
	// further are not needed.
	// 1. Decreasing width of chiclet panel.
	if (chiclet_panel_shrink_headrom > 0)
	{
		// we have some space to decrease chiclet panel
		S32 shrink_by = llmin(-delta_width, chiclet_panel_shrink_headrom);

		lldebugs << "delta_width: " << delta_width
			<< ", panel_delta_min: " << chiclet_panel_shrink_headrom
			<< ", shrink_by: " << shrink_by
			<< llendl;

		// is chiclet panel wide enough to process resizing?
		delta_width += chiclet_panel_shrink_headrom;

		still_should_be_processed = delta_width < 0;

		lldebugs << "Shrinking chiclet panel by " << shrink_by << " px" << llendl;
		mChicletPanel->getParent()->reshape(mChicletPanel->getParent()->getRect().getWidth() - shrink_by, mChicletPanel->getParent()->getRect().getHeight());
		log(mChicletPanel, "after processing panel decreasing via chiclet panel");

		lldebugs << "RS_CHICLET_PANEL" 
			<< ", delta_width: " << delta_width
			<< llendl;
	}

	S32 buttons_freed_width = 0;
	// 2. Decreasing width of buttons.
	if (still_should_be_processed)
	{
		processShrinkButtons(delta_width, buttons_freed_width);
	}
	// 3. Decreasing width of nearby chat.
	const S32 chatbar_panel_min_width = get_panel_min_width(mToolbarStack, mChatBarContainer);
	const S32 chatbar_panel_width = mChatBarContainer->getRect().getWidth();
	if (still_should_be_processed && chatbar_panel_width > chatbar_panel_min_width)
	{
		// we have some space to decrease chatbar panel
		S32 panel_delta_min = chatbar_panel_width - chatbar_panel_min_width;

		S32 delta_panel = llmin(-delta_width, panel_delta_min);

		// is chatbar panel wide enough to process resizing?
		delta_width += panel_delta_min;

		still_should_be_processed = delta_width < 0;

		// chatbar should only be shrunk here, not stretched
		if(delta_panel > 0)
		{
			lldebugs << "Shrinking nearby chat bar by " << delta_panel << " px " << llendl;
			mChatBarContainer->reshape(mNearbyChatBar->getRect().getWidth() - delta_panel, mChatBarContainer->getRect().getHeight());
		}

		log(mNearbyChatBar, "after processing panel decreasing via nearby chatbar panel");

		lldebugs << "RS_CHATBAR_INPUT"
			<< ", delta_panel: " << delta_panel
			<< ", delta_width: " << delta_width
			<< llendl;
	}

	S32 extra_shrink_width = 0;
	// 4. Hiding buttons if needed.
	if (still_should_be_processed)
	{
		processHideButtons(delta_width, buttons_freed_width);

		if (delta_width < 0)
		{
			extra_shrink_width = -delta_width;
			llwarns << "There is no enough width to reshape all children: " 
				<< extra_shrink_width << llendl;
		}

		if (buttons_freed_width > 0)
		{
			S32 nearby_needed_width = mDesiredNearbyChatWidth - mNearbyChatBar->getRect().getWidth();
			if (nearby_needed_width > 0)
			{
				S32 compensative_width = nearby_needed_width > buttons_freed_width ? buttons_freed_width : nearby_needed_width; 
				log(mNearbyChatBar, "before applying compensative width");
				lldebugs << "Extending nearby chat bar by " << compensative_width << " px" << llendl;
				mChatBarContainer->reshape(mChatBarContainer->getRect().getWidth() + compensative_width, mChatBarContainer->getRect().getHeight() );
				log(mNearbyChatBar, "after applying compensative width");
				lldebugs << buttons_freed_width << llendl;
			}
		}
	}

	return extra_shrink_width;
}

void LLBottomTray::processWidthIncreased(S32 delta_width)
{
	if (delta_width <= 0) return;

	// how much room we have to show hidden buttons
	S32 available_width = delta_width + getChicletPanelShrinkHeadroom();

	lldebugs << "Distributing (" << getChicletPanelShrinkHeadroom()
		<< " + " << delta_width << ") = " << available_width << " px" << llendl;

	S32 processed_width = processShowButtons(available_width);
	lldebugs << "processed_width = " << processed_width << ", delta_width = " << delta_width << llendl;

	lldebugs << "Available_width after showing buttons: " << available_width << llendl;

	// if we have to show/extend some buttons but resized delta width is not enough...
	if (processed_width > delta_width)
	{
		// ... let's shrink nearby chat & chiclet panels
		// 1. use delta width of resizing
		S32 shrink_by = processed_width - delta_width;

		// 2. use width available via decreasing of chiclet panel
		if (shrink_by > 0)
		{
			lldebugs << "Shrinking chiclet panel by " << shrink_by << " px" << llendl;
			mChicletPanel->getParent()->reshape(mChicletPanel->getParent()->getRect().getWidth() - shrink_by, mChicletPanel->getParent()->getRect().getHeight());
			log(mChicletPanel, "after applying compensative width for chiclets: ");
			lldebugs << shrink_by << llendl;
		}
	}

	// shown buttons take some space, rest should be processed by nearby chatbar & chiclet panels
	delta_width -= processed_width;

	// how much space can nearby chatbar take?
	S32 chatbar_panel_width = mChatBarContainer->getRect().getWidth();
	lldebugs << "delta_width = " << delta_width
		<< ", chatbar_panel_width = " << chatbar_panel_width
		<< ", mDesiredNearbyChatWidth = " << mDesiredNearbyChatWidth << llendl;
	if (delta_width > 0 && chatbar_panel_width < mDesiredNearbyChatWidth)
	{
		S32 delta_panel_max = mDesiredNearbyChatWidth - chatbar_panel_width;
		S32 delta_panel = llmin(delta_width, delta_panel_max);
		lldebugs << "Unprocesed delta width: " << delta_width
			<< ", can be applied to chatbar: " << delta_panel_max
			<< ", will be applied: " << delta_panel
			<< llendl;

		delta_width -= delta_panel_max;
		lldebugs << "Extending nearby chat bar by " << delta_panel << " px " << llendl;
		mChatBarContainer->reshape(chatbar_panel_width + delta_panel, mChatBarContainer->getRect().getHeight());
		log(mNearbyChatBar, "applied unprocessed delta width");
	}

	if (delta_width > 0)
	{
		processExtendButtons(delta_width);
	}
}

S32 LLBottomTray::processShowButtons(S32& available_width)
{
	lldebugs << "Distributing " << available_width << " px" << llendl;
	S32 original_available_width = available_width;

	// process buttons from left to right
	resize_state_vec_t::const_iterator it = mButtonsProcessOrder.begin();
	const resize_state_vec_t::const_iterator it_end = mButtonsProcessOrder.end();

	for (; it != it_end; ++it)
	{
		// is there available space?
		if (available_width <= 0) break;

		// try to show next button
		processShowButton(*it, available_width);
	}

	return original_available_width - available_width;
}

bool LLBottomTray::processShowButton(EResizeState shown_object_type, S32& available_width)
{
	// Check if the button was previously auto-hidden (due to lack of space).
	if ((mResizeState & shown_object_type) == 0)
	{
		return false;
	}

	// Ok. Try showing the button.
	return showButton(shown_object_type, available_width);
}

void LLBottomTray::processHideButtons(S32& required_width, S32& buttons_freed_width)
{
	// process buttons from right to left
	resize_state_vec_t::const_reverse_iterator it = mButtonsProcessOrder.rbegin();
	const resize_state_vec_t::const_reverse_iterator it_end = mButtonsProcessOrder.rend();

	for (; it != it_end; ++it)
	{
		// is it still necessary to hide a button?
		if (required_width >= 0) break;

		// try to hide next button
		processHideButton(*it, required_width, buttons_freed_width);
	}
}

void LLBottomTray::processHideButton(EResizeState processed_object_type, S32& required_width, S32& buttons_freed_width)
{
	lldebugs << "Trying to hide object type: " << processed_object_type << llendl;
	LLPanel* panel = getButtonPanel(processed_object_type);
	if (NULL == panel)
	{
		return;
	}

	if (panel->getVisible())
	{
		required_width += panel->getRect().getWidth();

		if (required_width > 0)
		{
			buttons_freed_width += required_width;
		}

		setTrayButtonVisible(processed_object_type, false);

		mResizeState |= processed_object_type;

		lldebugs << "processing object type: " << processed_object_type
			<< ", buttons_freed_width: " << buttons_freed_width
			<< llendl;
	}
}

void LLBottomTray::processShrinkButtons(S32& required_width, S32& buttons_freed_width)
{
	// process buttons from right to left
	resize_state_vec_t::const_reverse_iterator it = mButtonsProcessOrder.rbegin();
	const resize_state_vec_t::const_reverse_iterator it_end = mButtonsProcessOrder.rend();

	// iterate through buttons in the mButtonsProcessOrder first
	for (; it != it_end; ++it)
	{
		// is it still necessary to hide a button?
		if (required_width >= 0) break;

		// try to shrink next button
		processShrinkButton(*it, required_width);
	}

	// then shrink Speak button
	if (required_width < 0)
	{
		S32 panel_min_width = 0;
		std::string panel_name = mSpeakPanel->getName();
		bool success = mToolbarStack->getPanelMinSize(panel_name, &panel_min_width);
		if (!success)
		{
			lldebugs << "Panel was not found to get its min width: " << panel_name << llendl;
		}
		else
		{
			S32 panel_width = mSpeakPanel->getRect().getWidth();
			S32 possible_shrink_width = panel_width - panel_min_width;

			if (possible_shrink_width > 0)
			{
				mSpeakBtn->setLabelVisible(false);
				mSpeakPanel->reshape(panel_width - possible_shrink_width, mSpeakPanel->getRect().getHeight());

				required_width += possible_shrink_width;

				if (required_width > 0)
				{
					buttons_freed_width += required_width;
				}

				lldebugs << "Shrunk Speak button panel: " << panel_name
					<< ", shrunk width: " << possible_shrink_width
					<< ", rest width to process: " << required_width
					<< llendl;
			}
		}
	}
}

void LLBottomTray::processShrinkButton(EResizeState processed_object_type, S32& required_width)
{
	LLPanel* panel = getButtonPanel(processed_object_type);
	if (NULL == panel)
	{
		return;
	}

	if (panel->getVisible())
	{
		S32 panel_width = panel->getRect().getWidth();
		S32 panel_min_width = 0;
		std::string panel_name = panel->getName();
		bool success = mToolbarStack->getPanelMinSize(panel_name, &panel_min_width);
		S32 possible_shrink_width = panel_width - panel_min_width;

		if (!success)
		{
			lldebugs << "Panel was not found to get its min width: " << panel_name << llendl;
		}
		// we have some space to free by shrinking the button
		else if (possible_shrink_width > 0)
		{
			// let calculate real width to shrink

			// 1. apply all possible width
			required_width += possible_shrink_width;

			// 2. it it is too much... 
			if (required_width > 0)
			{
				// reduce applied shrunk width to the excessive value.
				possible_shrink_width -= required_width;
				required_width = 0;
			}
			panel->reshape(panel_width - possible_shrink_width, panel->getRect().getHeight());

			lldebugs << "Shrunk panel: " << panel_name
				<< ", shrunk width: " << possible_shrink_width
				<< ", rest width to process: " << required_width
				<< llendl;
		}
	}
}


void LLBottomTray::processExtendButtons(S32& available_width)
{
	// do not allow extending any buttons if we have some buttons hidden via resize
	if (mResizeState & RS_BUTTONS_CAN_BE_HIDDEN) return;

	lldebugs << "Distributing " << available_width << " px" << llendl;

	// First try extending the Speak button.
	if (available_width > 0)
	{
		if (!processExtendSpeakButton(available_width))
		{
			// The Speak button needs extension but lacks some space.
			// Don't extend other buttons in this case: the Speak button
			// should consume the available headroom first.
			return;
		}
	}

	// Then process the other buttons from left to right.
	if (available_width > 0)
	{
		resize_state_vec_t::const_iterator it = mButtonsProcessOrder.begin();
		const resize_state_vec_t::const_iterator it_end = mButtonsProcessOrder.end();

		// iterate through buttons in the mButtonsProcessOrder first
		for (; it != it_end; ++it)
		{
			// is there available space?
			if (available_width <= 0) break;

			// try to extend next button
			processExtendButton(*it, available_width);
		}
	}
}

bool LLBottomTray::processExtendSpeakButton(S32& available_width)
{
	if (available_width <= 0)
	{
		llassert(available_width > 0);
		return true;
	}

	const S32 panel_max_width = mObjectDefaultWidthMap[RS_BUTTON_SPEAK];
	const S32 panel_width = mSpeakPanel->getRect().getWidth();
	const S32 required_headroom = panel_max_width - panel_width;

#if 0
	lldebugs << "required_extend_width = (panel_max_width - panel_width) = "
		<< "(" << panel_max_width << " - " << panel_width << ") = " << required_headroom << llendl;
#endif

	if (panel_width < panel_max_width) // if the button isn't extended already
	{
		if (available_width < required_headroom) // not enough space
		{
			lldebugs << "Need (" << required_headroom << " - " << available_width << ") = "
				<< (required_headroom - available_width) << " more px"
				<< " to extend the Speak button"<< llendl;

			return false; // Don't extend other buttons until we extend Speak.
		}

		// Reshape the Speak button to its maximum width.
		mSpeakBtn->setLabelVisible(true);
		mSpeakPanel->reshape(panel_max_width, mSpeakPanel->getRect().getHeight());

		available_width -= required_headroom;
		llassert(available_width >= 0);

		lldebugs << "Extending Speak button panel: " << mSpeakPanel->getName()
			<< ", extended width: " << required_headroom
			<< ", rest width to process: " << available_width
			<< llendl;
	}

	return true;
}

void LLBottomTray::processExtendButton(EResizeState processed_object_type, S32& available_width)
{
	llassert(available_width >= 0);

	LLPanel* panel = getButtonPanel(processed_object_type);
	if (NULL == panel)
	{
		return;
	}

	if (!panel->getVisible()) return;

	// Widen the button up to its maximum width, but by not more than <available_width> px.
	S32 panel_max_width = mObjectDefaultWidthMap[processed_object_type];
	S32 panel_width = panel->getRect().getWidth();
	S32 required_headroom = panel_max_width - panel_width;

	S32 extend_by = llmin(available_width, required_headroom);
	if (extend_by > 0)
	{
		panel->reshape(panel_width + extend_by, panel->getRect().getHeight());

		// Decrease amount of headroom available for other panels.
		available_width -= extend_by;

		lldebugs << "Extending " << panel->getName()
			<< " by " << extend_by
			<< " px; remaining available width: " << available_width
			<< llendl;
	}
}

bool LLBottomTray::canButtonBeShown(EResizeState processed_object_type) const
{
	// 1. Let's check that all buttons (that can be hidden on resize) before the given one are already shown.

	// process buttons in direct order (from left to right)
	resize_state_vec_t::const_iterator it = mButtonsProcessOrder.begin();
	const resize_state_vec_t::const_iterator it_end = mButtonsProcessOrder.end();

	// 1. Find and accumulate all buttons types before one passed into the method.
	MASK buttons_before_mask = RS_NORESIZE;
	for (; it != it_end; ++it)
	{
		const EResizeState button_type = *it;
		if (button_type == processed_object_type) break;

		buttons_before_mask |= button_type;
	}

	// 2. Check if some previous buttons are still hidden on resize
	bool can_be_shown = !(buttons_before_mask & mResizeState);
#if 0
	if (!can_be_shown)
	{
		lldebugs << llformat("mResizeState = 0x%4x, buttons_before_mask = 0x%4x", mResizeState,  buttons_before_mask) << llendl;
		lldebugs << "Auto-hidden: " << resizeStateMaskToString(mResizeState) << llendl;
		lldebugs << "Must show the following buttons to the left of " << resizeStateToString(processed_object_type) << " first:" << llendl;
		lldebugs << resizeStateMaskToString(buttons_before_mask & mResizeState) << llendl;
	}
#endif
	return can_be_shown;
}

void LLBottomTray::initResizeStateContainers()
{
	// init map with objects should be processed for each type
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_SPEAK, getChild<LLPanel>("speak_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_GESTURES, getChild<LLPanel>("gesture_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_MOVEMENT, getChild<LLPanel>("movement_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_CAMERA, getChild<LLPanel>("cam_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_SNAPSHOT, getChild<LLPanel>("snapshot_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_BUILD, getChild<LLPanel>("build_btn_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_SEARCH, getChild<LLPanel>("search_btn_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_WORLD_MAP, getChild<LLPanel>("world_map_btn_panel")));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_MINI_MAP, getChild<LLPanel>("mini_map_btn_panel")));

	// init an order of processed buttons
	mButtonsProcessOrder.push_back(RS_BUTTON_GESTURES);
	mButtonsProcessOrder.push_back(RS_BUTTON_MOVEMENT);
	mButtonsProcessOrder.push_back(RS_BUTTON_CAMERA);
	mButtonsProcessOrder.push_back(RS_BUTTON_SNAPSHOT);
	mButtonsProcessOrder.push_back(RS_BUTTON_BUILD);
	mButtonsProcessOrder.push_back(RS_BUTTON_SEARCH);
	mButtonsProcessOrder.push_back(RS_BUTTON_WORLD_MAP);
	mButtonsProcessOrder.push_back(RS_BUTTON_MINI_MAP);

	mButtonsOrder.push_back(RS_BUTTON_SPEAK);
	mButtonsOrder.insert(mButtonsOrder.end(), mButtonsProcessOrder.begin(), mButtonsProcessOrder.end());

	// init default widths

	// process buttons that can be hidden on resize...
	resize_state_vec_t::const_iterator it = mButtonsProcessOrder.begin();
	const resize_state_vec_t::const_iterator it_end = mButtonsProcessOrder.end();

	for (; it != it_end; ++it)
	{
		const EResizeState button_type = *it;
		// is there an appropriate object?
		LLPanel* button_panel = getButtonPanel(button_type);
		if (!button_panel) continue;

		// set default width for it.
		mObjectDefaultWidthMap[button_type] = button_panel->getRect().getWidth();
	}

	// ... and add Speak button because it also can be shrunk.
	mObjectDefaultWidthMap[RS_BUTTON_SPEAK]	   = mSpeakPanel->getRect().getWidth();
}

// this method must be called before restoring of the chat entry field on startup
// because it resets chatbar's width according to resize logic.
void LLBottomTray::initButtonsVisibility()
{
	setVisibleAndFitWidths(RS_BUTTON_SPEAK, gSavedSettings.getBOOL("EnableVoiceChat"));
	setVisibleAndFitWidths(RS_BUTTON_GESTURES, gSavedSettings.getBOOL("ShowGestureButton"));
	setVisibleAndFitWidths(RS_BUTTON_MOVEMENT, gSavedSettings.getBOOL("ShowMoveButton"));
	setVisibleAndFitWidths(RS_BUTTON_CAMERA, gSavedSettings.getBOOL("ShowCameraButton"));
	setVisibleAndFitWidths(RS_BUTTON_SNAPSHOT, gSavedSettings.getBOOL("ShowSnapshotButton"));
	setVisibleAndFitWidths(RS_BUTTON_BUILD, gSavedSettings.getBOOL("ShowBuildButton"));
	setVisibleAndFitWidths(RS_BUTTON_SEARCH, gSavedSettings.getBOOL("ShowSearchButton"));
	setVisibleAndFitWidths(RS_BUTTON_WORLD_MAP, gSavedSettings.getBOOL("ShowWorldMapButton"));
	setVisibleAndFitWidths(RS_BUTTON_MINI_MAP, gSavedSettings.getBOOL("ShowMiniMapButton"));
	lldebugs << "mResizeState = " << resizeStateMaskToString(mResizeState) << llendl;
}

void LLBottomTray::setButtonsControlsAndListeners()
{
	gSavedSettings.getControl("EnableVoiceChat")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_SPEAK, _2));
	gSavedSettings.getControl("ShowGestureButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_GESTURES, _2));
	gSavedSettings.getControl("ShowMoveButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_MOVEMENT, _2));
	gSavedSettings.getControl("ShowCameraButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_CAMERA, _2));
	gSavedSettings.getControl("ShowSnapshotButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_SNAPSHOT, _2));
	gSavedSettings.getControl("ShowBuildButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_BUILD, _2));
	gSavedSettings.getControl("ShowSearchButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_SEARCH, _2));
	gSavedSettings.getControl("ShowWorldMapButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_WORLD_MAP, _2));
	gSavedSettings.getControl("ShowMiniMapButton")->getSignal()->connect(boost::bind(&LLBottomTray::toggleShowButton, RS_BUTTON_MINI_MAP, _2));


	LLButton* build_btn = getChild<LLButton>("build_btn");
	// set control name for Build button. It is not enough to link it with Button.SetFloaterToggle in xml
	std::string vis_control_name = LLFloaterReg::declareVisibilityControl("build");
	// Set the button control value (toggle state) to the floater visibility control (Sets the value as well)
	build_btn->setControlVariable(LLFloater::getControlGroup()->getControl(vis_control_name));
}

bool LLBottomTray::toggleShowButton(LLBottomTray::EResizeState button_type, const LLSD& new_visibility)
{
	if (LLBottomTray::instanceExists())
	{
		LLBottomTray::getInstance()->setTrayButtonVisibleIfPossible(button_type, new_visibility.asBoolean());
	}
	return true;
}

bool LLBottomTray::showButton(EResizeState button_type, S32& available_width)
{
	LLPanel* panel = getButtonPanel(button_type);
	if (NULL == panel)
	{
		return false;
	}

	if (panel->getVisible())
	{
		return false;
	}

	// Check if none of the buttons to the left of the given one was auto-hidden.
	// (we auto-show the buttons left to right).
	if (!canButtonBeShown(button_type))
	{
		return false;
	}

	// Make sure we have enough room to show this button.
	const S32 required_width = panel->getRect().getWidth();
	if (available_width < required_width)
	{
		lldebugs << "Need " << (required_width - available_width) << " more px to show " << resizeStateToString(button_type) << llendl;
		return false;
	}

	// All good. Show the button.
	setTrayButtonVisible(button_type, true);

	// Let the caller know that there is now less available space.
	available_width -= required_width;

	lldebugs << "Showing button " << resizeStateToString(button_type)
		<< ", remaining available width: " << available_width
		<< llendl;
	mResizeState &= ~button_type;

	return true;
}

void LLBottomTray::setTrayButtonVisible(EResizeState shown_object_type, bool visible)
{
	LLPanel* panel = getButtonPanel(shown_object_type);
	if (NULL == panel)
	{
		return;
	}

	panel->setVisible(visible);
}

void LLBottomTray::setTrayButtonVisibleIfPossible(EResizeState shown_object_type, bool visible, bool raise_notification)
{
	if (!setVisibleAndFitWidths(shown_object_type, visible) && visible && raise_notification)
	{
		LLNotificationsUtil::add("BottomTrayButtonCanNotBeShown",
								 LLSD(),
								 LLSD(),
								 LLNotificationFunctorRegistry::instance().DONOTHING);
	}
}

bool LLBottomTray::setVisibleAndFitWidths(EResizeState object_type, bool visible)
{
	// The Speak button is treated specially: if voice is enabled,
	// the button should be displayed no matter how much space we've got.
	if (object_type == RS_BUTTON_SPEAK)
	{
		showSpeakButton(visible);
		return true;
	}

	LLPanel* cur_panel = getButtonPanel(object_type);
	if (NULL == cur_panel)
	{
		return false;
	}

	bool is_set = true;

	if (visible)
	{
		// Assume that only chiclet panel can be auto-resized
		const S32 available_width = getChicletPanelShrinkHeadroom();

		S32 preferred_width = mObjectDefaultWidthMap[object_type];
		S32 current_width = cur_panel->getRect().getWidth();
		S32 result_width = 0;
		bool decrease_width = false;

#if 0
		// Mark this button to be shown
		lldebugs << "Adding " << resizeStateToString(object_type) << " to mResizeState" << llendl;
		mResizeState |= object_type;
#endif

		if (preferred_width > 0 && available_width >= preferred_width)
		{
			result_width = preferred_width;
		}
		else if (available_width >= current_width)
		{
			result_width = current_width;
		}
		else
		{
			// Calculate the possible shrunk width as difference between current and minimal widths
			const S32 chatbar_shrunk_width =
				mChatBarContainer->getRect().getWidth() - get_panel_min_width(mToolbarStack, mChatBarContainer);

			S32 sum_of_min_widths = get_panel_min_width(mToolbarStack, mSpeakPanel);
			S32 sum_of_curr_widths = get_curr_width(mSpeakPanel);

			resize_state_vec_t::const_iterator it = mButtonsProcessOrder.begin();
			const resize_state_vec_t::const_iterator it_end = mButtonsProcessOrder.end();

			for (; it != it_end; ++it)
			{
				LLPanel* cur_panel = getButtonPanel(*it);
				sum_of_min_widths += get_panel_min_width(mToolbarStack, cur_panel);
				sum_of_curr_widths += get_curr_width(cur_panel);
			}

			const S32 possible_shrunk_width =
				chatbar_shrunk_width + (sum_of_curr_widths - sum_of_min_widths);

			// Minimal width of current panel
			S32 minimal_width = 0;
			mToolbarStack->getPanelMinSize(cur_panel->getName(), &minimal_width);

			if ( (available_width + possible_shrunk_width) >= minimal_width)
			{
				// There is enough space for minimal width, but set the result_width
				// to preferred_width so buttons widths decreasing will be done in predefined order
				result_width = (preferred_width > 0) ? preferred_width : current_width;
				decrease_width = true;
			}
			else
			{
				// Nothing can be done, give up...
				return false;
			}
		}

		if (result_width != current_width)
		{
			cur_panel->reshape(result_width, cur_panel->getRect().getHeight());
			current_width = result_width;
		}

		is_set = showButton(object_type, current_width);

		// Shrink buttons if needed
		if (is_set && decrease_width)
		{
			processWidthDecreased( -result_width);
		}
	}
	else
	{
		const S32 delta_width = get_curr_width(cur_panel);

		setTrayButtonVisible(object_type, false);

		// Mark button NOT to show while future bottom tray extending
		lldebugs << "Removing " << resizeStateToString(object_type) << " from mResizeState" << llendl;
		mResizeState &= ~object_type;

		// Extend other buttons if need
		if (delta_width)
		{
			processWidthIncreased(delta_width);
		}
	}
	return is_set;
}

LLPanel* LLBottomTray::getButtonPanel(EResizeState button_type)
{
	// Don't use the operator[] because it inserts a NULL value if the key is not found.
	if (mStateProcessedObjectMap.count(button_type) == 0)
	{
		llwarns << "Cannot find a panel for " << resizeStateToString(button_type) << llendl;
		llassert(mStateProcessedObjectMap.count(button_type) == 1);
		return NULL;
	}

	return mStateProcessedObjectMap[button_type];
}

void LLBottomTray::showWellButton(EResizeState object_type, bool visible)
{
	llassert( ((RS_NOTIFICATION_WELL | RS_IM_WELL) & object_type) == object_type );

	const std::string panel_name = RS_IM_WELL == object_type ? "im_well_panel" : "notification_well_panel";

	LLView * panel = getChild<LLView>(panel_name);

	// if necessary visibility is set nothing to do here
	if (panel->getVisible() == (BOOL)visible) return;

	S32 panel_width = panel->getRect().getWidth();
	panel->setVisible(visible);

	if (visible)
	{
		// method assumes that input param is a negative value
		processWidthDecreased(-panel_width);
	}
	else
	{
		processWidthIncreased(panel_width);
	}
}

void LLBottomTray::processChatbarCustomization(S32 new_width)
{
	if (NULL == mNearbyChatBar) return;

	const S32 delta_width = mChatBarContainer->getRect().getWidth() - new_width;

	if (delta_width == 0) return;

	{
		static unsigned dbg_cnt = 0;
		lldebugs << llformat("*** (%03d) ************************************* %d", delta_width, ++dbg_cnt) << llendl;
	}

	mDesiredNearbyChatWidth = new_width;

	const S32 available_chiclet_shrink_width = getChicletPanelShrinkHeadroom();
	llassert(available_chiclet_shrink_width >= 0);

	if (delta_width > 0) // panel gets narrowly
	{
		S32 total_possible_width = delta_width + available_chiclet_shrink_width;
		processShowButtons(total_possible_width);
		processExtendButtons(total_possible_width);
	}
	// here (delta_width < 0) // panel gets wider
	else //if (-delta_width > available_chiclet_shrink_width)
	{
		S32 required_width = delta_width + available_chiclet_shrink_width;
		S32 buttons_freed_width = 0;
		processShrinkButtons(required_width, buttons_freed_width);
		processHideButtons(required_width, buttons_freed_width);
	}
}

S32 LLBottomTray::getChicletPanelShrinkHeadroom() const
{
	static const S32 min_width = mChicletPanel->getMinWidth();
	const S32 cur_width = mChicletPanel->getParent()->getRect().getWidth();

	S32 shrink_headroom = cur_width - min_width;
	llassert(shrink_headroom >= 0); // the panel cannot get narrower than the minimum
	return shrink_headroom;
}

// static
std::string LLBottomTray::resizeStateToString(EResizeState state)
{
	switch (state)
	{
	case RS_NORESIZE:				return "RS_NORESIZE";
	case RS_CHICLET_PANEL:			return "RS_CHICLET_PANEL";
	case RS_CHATBAR_INPUT:			return "RS_CHATBAR_INPUT";
	case RS_BUTTON_SNAPSHOT:		return "RS_BUTTON_SNAPSHOT";
	case RS_BUTTON_CAMERA:			return "RS_BUTTON_CAMERA";
	case RS_BUTTON_MOVEMENT:		return "RS_BUTTON_MOVEMENT";
	case RS_BUTTON_GESTURES:		return "RS_BUTTON_GESTURES";
	case RS_BUTTON_SPEAK:			return "RS_BUTTON_SPEAK";
	case RS_IM_WELL:				return "RS_IM_WELL";
	case RS_NOTIFICATION_WELL:		return "RS_NOTIFICATION_WELL";
	case RS_BUTTON_BUILD:			return "RS_BUTTON_BUILD";
	case RS_BUTTON_SEARCH:			return "RS_BUTTON_SEARCH";
	case RS_BUTTON_WORLD_MAP:		return "RS_BUTTON_WORLD_MAP";
	case RS_BUTTON_MINI_MAP:		return "RS_BUTTON_MINI_MAP";
	case RS_BUTTONS_CAN_BE_HIDDEN:	return "RS_BUTTONS_CAN_BE_HIDDEN";
	// No default to track additions.
	}
	return "UNKNOWN_BUTTON";
}

// static
std::string LLBottomTray::resizeStateMaskToString(MASK mask)
{
	std::string res;

	bool add_delimiter = false;
    for (U32 i = 0; i < 16; i++)
    {
    	EResizeState state = (EResizeState) (1 << i);
    	if (mask & state)
    	{
    		if (!add_delimiter)
    		{
    			add_delimiter = true;
    		}
    		else
    		{
    			res += ", ";
    		}

			res += resizeStateToString(state);
    	}
    }

    if (res.empty())
    {
    	res = resizeStateToString(RS_NORESIZE);
    }

    res += llformat(" (0x%X)", mask);
    return res;
}

//EOF
