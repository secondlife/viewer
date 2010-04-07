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

#define LLBOTTOMTRAY_CPP
#include "llbottomtray.h"

#include "llagentcamera.h"
#include "llchiclet.h"
#include "llfloaterreg.h"
#include "llflyoutbutton.h"
#include "llimfloater.h" // for LLIMFloater
#include "lllayoutstack.h"
#include "llnearbychatbar.h"
#include "llnotificationsutil.h"
#include "llspeakbutton.h"
#include "llsplitbutton.h"
#include "llsyswellwindow.h"
#include "llfloatercamera.h"
#include "lltexteditor.h"
#include "llnotifications.h"

// Build time optimization, generate extern template once in .cpp file
template class LLBottomTray* LLSingleton<class LLBottomTray>::getInstance();

namespace
{
	const std::string& PANEL_CHICLET_NAME	= "chiclet_list_panel";
	const std::string& PANEL_CHATBAR_NAME	= "chat_bar";
	const std::string& PANEL_MOVEMENT_NAME	= "movement_panel";
	const std::string& PANEL_CAMERA_NAME	= "cam_panel";
	const std::string& PANEL_GESTURE_NAME	= "gesture_panel";

	S32 get_panel_min_width(LLLayoutStack* stack, LLPanel* panel)
	{
		S32 minimal_width = 0;
		llassert(stack);
		if ( stack && panel && panel->getVisible() )
		{
			stack->getPanelMinSize(panel->getName(), &minimal_width, NULL);
		}
		return minimal_width;
	}

	S32 get_panel_max_width(LLLayoutStack* stack, LLPanel* panel)
	{
		S32 max_width = 0;
		llassert(stack);
		if ( stack && panel && panel->getVisible() )
		{
			stack->getPanelMaxSize(panel->getName(), &max_width, NULL);
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
		  mGesturePanel(NULL)
	{
		mFactoryMap["chat_bar"] = LLCallbackMap(LLBottomTray::createNearbyChatBar, NULL);
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_bottomtray_lite.xml");
		// Necessary for focus movement among child controls
		setFocusRoot(TRUE);
	}

	BOOL postBuild()
	{
		mNearbyChatBar = getChild<LLNearbyChatBar>("chat_bar");
		mGesturePanel = getChild<LLPanel>("gesture_panel");

		// Hide "show_nearby_chat" button 
		LLLineEditor* chat_box = mNearbyChatBar->getChatBox();
		LLUICtrl* show_btn = mNearbyChatBar->getChild<LLUICtrl>("show_nearby_chat");
		S32 delta_width = show_btn->getRect().getWidth();
		show_btn->setVisible(FALSE);
		chat_box->reshape(chat_box->getRect().getWidth() + delta_width, chat_box->getRect().getHeight());

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
	LLPanel*			mGesturePanel;
};

LLBottomTray::LLBottomTray(const LLSD&)
:	mChicletPanel(NULL),
	mSpeakPanel(NULL),
	mSpeakBtn(NULL),
	mNearbyChatBar(NULL),
	mToolbarStack(NULL)
,	mMovementButton(NULL)
,	mResizeState(RS_NORESIZE)
,	mBottomTrayContextMenu(NULL)
,	mMovementPanel(NULL)
,	mCamPanel(NULL)
,	mSnapshotPanel(NULL)
,	mGesturePanel(NULL)
,	mCamButton(NULL)
,	mBottomTrayLite(NULL)
,	mIsInLiteMode(false)
{
	// Firstly add ourself to IMSession observers, so we catch session events
	// before chiclets do that.
	LLIMMgr::getInstance()->addSessionObserver(this);

	mFactoryMap["chat_bar"] = LLCallbackMap(LLBottomTray::createNearbyChatBar, NULL);

	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_bottomtray.xml");

	LLUICtrl::CommitCallbackRegistry::defaultRegistrar().add("CameraPresets.ChangeView", boost::bind(&LLFloaterCamera::onClickCameraPresets, _2));

	//this is to fix a crash that occurs because LLBottomTray is a singleton
	//and thus is deleted at the end of the viewers lifetime, but to be cleanly
	//destroyed LLBottomTray requires some subsystems that are long gone
	//LLUI::getRootView()->addChild(this);

	// Necessary for focus movement among child controls
	setFocusRoot(TRUE);

	{
		mBottomTrayLite = new LLBottomTrayLite();
		mBottomTrayLite->setFollowsAll();
		mBottomTrayLite->setVisible(FALSE);
	}
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
		mSpeakBtn->setFlyoutBtnEnabled(LLVoiceClient::voiceEnabled() && gVoiceClient->voiceWorking());
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

	S32 local_x = x - mNearbyChatBar->getRect().mLeft - edit_box->getRect().mLeft;
	S32 local_y = y - mNearbyChatBar->getRect().mBottom - edit_box->getRect().mBottom;

	bool in_edit_box = edit_box->pointInView(local_x, local_y);

	LLMenuItemGL* menu_item;
	menu_item = mBottomTrayContextMenu->findChild<LLMenuItemGL>("NearbyChatBar_Cut");
	if(menu_item)
		menu_item->setVisible(in_edit_box);
	menu_item = mBottomTrayContextMenu->findChild<LLMenuItemGL>("NearbyChatBar_Copy");
	if(menu_item)
		menu_item->setVisible(in_edit_box);
	menu_item = mBottomTrayContextMenu->findChild<LLMenuItemGL>("NearbyChatBar_Paste");
	if(menu_item)
		menu_item->setVisible(in_edit_box);
	menu_item = mBottomTrayContextMenu->findChild<LLMenuItemGL>("NearbyChatBar_Delete");
	if(menu_item)
		menu_item->setVisible(in_edit_box);
	menu_item = mBottomTrayContextMenu->findChild<LLMenuItemGL>("NearbyChatBar_Select_All");
	if(menu_item)
		menu_item->setVisible(in_edit_box);
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

	LLUICtrl::CommitCallbackRegistry::currentRegistrar().add("NearbyChatBar.Action", boost::bind(&LLBottomTray::onContextMenuItemClicked, this, _2));
	LLUICtrl::EnableCallbackRegistry::currentRegistrar().add("NearbyChatBar.EnableMenuItem", boost::bind(&LLBottomTray::onContextMenuItemEnabled, this, _2));

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
	gVoiceClient->addObserver(this);

	mObjectDefaultWidthMap[RS_BUTTON_GESTURES] = mGesturePanel->getRect().getWidth();
	mObjectDefaultWidthMap[RS_BUTTON_MOVEMENT] = mMovementPanel->getRect().getWidth();
	mObjectDefaultWidthMap[RS_BUTTON_CAMERA]   = mCamPanel->getRect().getWidth();
	mObjectDefaultWidthMap[RS_BUTTON_SPEAK]	   = mSpeakPanel->getRect().getWidth();

	mNearbyChatBar->getChatBox()->setContextMenu(NULL);

	mChicletPanel = getChild<LLChicletPanel>("chiclet_list");
	mChicletPanel->setChicletClickedCallback(boost::bind(&LLBottomTray::onChicletClick,this,_1));

	initStateProcessedObjectMap();

	// update wells visibility:
	showWellButton(RS_IM_WELL, !LLIMWellWindow::getInstance()->isWindowEmpty());
	showWellButton(RS_NOTIFICATION_WELL, !LLNotificationWellWindow::getInstance()->isWindowEmpty());

	return TRUE;
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
 
 
		<< "layout: " << layout->getName()
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

	// stores width size on which bottom tray is less than width required by its children. EXT-991
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
			if (extra_shrink_width > 0)
			{
				// is world rect was extra shrunk and decreasing again only update this value
				// to delta_width negative
				extra_shrink_width -= delta_width; // use "-=" because delta_width is negative
				should_be_reshaped = false;
			}
			else
			{
				extra_shrink_width = processWidthDecreased(delta_width);

				// increase new width to extra_shrink_width value to not reshape less than bottom tray minimum
				width += extra_shrink_width;
			}
		}
		// bottom tray is widen
		else
		{
			if (extra_shrink_width > delta_width)
			{
				// Less than minimum width is more than increasing (delta_width) 
				// only reduce it value and make no reshape
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
}

S32 LLBottomTray::processWidthDecreased(S32 delta_width)
{
	bool still_should_be_processed = true;

	const S32 chiclet_panel_width = mChicletPanel->getParent()->getRect().getWidth();
	const S32 chiclet_panel_min_width = mChicletPanel->getMinWidth();

	if (chiclet_panel_width > chiclet_panel_min_width)
	{
		// we have some space to decrease chiclet panel
		S32 panel_delta_min = chiclet_panel_width - chiclet_panel_min_width;

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

	const S32 chatbar_panel_width = mNearbyChatBar->getRect().getWidth();
	const S32 chatbar_panel_min_width = get_panel_min_width(mToolbarStack, mNearbyChatBar);
	if (still_should_be_processed && chatbar_panel_width > chatbar_panel_min_width)
	{
		// we have some space to decrease chatbar panel
		S32 panel_delta_min = chatbar_panel_width - chatbar_panel_min_width;

		S32 delta_panel = llmin(-delta_width, panel_delta_min);

		// whether chatbar panel width is enough to process resizing?
		delta_width += panel_delta_min;

		still_should_be_processed = delta_width < 0;

		mNearbyChatBar->reshape(mNearbyChatBar->getRect().getWidth() - delta_panel, mNearbyChatBar->getRect().getHeight());

		log(mNearbyChatBar, "after processing panel decreasing via nearby chatbar panel");

		lldebugs << "RS_CHATBAR_INPUT"
			<< ", delta_panel: " << delta_panel
			<< ", delta_width: " << delta_width
			<< llendl;
	}

	S32 extra_shrink_width = 0;
	S32 buttons_freed_width = 0;
	if (still_should_be_processed)
	{
		processShrinkButtons(&delta_width, &buttons_freed_width);

		if (delta_width < 0)
		{
			processHideButton(RS_BUTTON_SNAPSHOT, &delta_width, &buttons_freed_width);
		}

		if (delta_width < 0)
		{
			processHideButton(RS_BUTTON_CAMERA, &delta_width, &buttons_freed_width);
		}

		if (delta_width < 0)
		{
			processHideButton(RS_BUTTON_MOVEMENT, &delta_width, &buttons_freed_width);
		}

		if (delta_width < 0)
		{
			processHideButton(RS_BUTTON_GESTURES, &delta_width, &buttons_freed_width);
		}

		if (delta_width < 0)
		{
			extra_shrink_width = -delta_width;
			llwarns << "There is no enough width to reshape all children: " 
				<< extra_shrink_width << llendl;
		}

		if (buttons_freed_width > 0)
		{
			log(mNearbyChatBar, "before applying compensative width");
			mNearbyChatBar->reshape(mNearbyChatBar->getRect().getWidth() + buttons_freed_width, mNearbyChatBar->getRect().getHeight() );
			log(mNearbyChatBar, "after applying compensative width");
			lldebugs << buttons_freed_width << llendl;
		}
	}

	return extra_shrink_width;
}

void LLBottomTray::processWidthIncreased(S32 delta_width)
{
	if (delta_width <= 0) return;

	const S32 chiclet_panel_width = mChicletPanel->getParent()->getRect().getWidth();
	static const S32 chiclet_panel_min_width = mChicletPanel->getMinWidth();

	const S32 chatbar_panel_width = mNearbyChatBar->getRect().getWidth();
	static const S32 chatbar_panel_min_width = get_panel_min_width(mToolbarStack, mNearbyChatBar);
	static const S32 chatbar_panel_max_width = get_panel_max_width(mToolbarStack, mNearbyChatBar);

	const S32 chatbar_available_shrink_width = chatbar_panel_width - chatbar_panel_min_width;
	const S32 available_width_chiclet = chiclet_panel_width - chiclet_panel_min_width;

	// how many room we have to show hidden buttons
	S32 total_available_width = delta_width + chatbar_available_shrink_width + available_width_chiclet;

	lldebugs << "Processing extending, available width:"
		<< ", chatbar - " << chatbar_available_shrink_width
		<< ", chiclets - " << available_width_chiclet
		<< ", total - " << total_available_width
		<< llendl;

	S32 available_width = total_available_width;
	if (available_width > 0)
	{
		processShowButton(RS_BUTTON_GESTURES, &available_width);
	}

	if (available_width > 0)
	{
		processShowButton(RS_BUTTON_MOVEMENT, &available_width);
	}

	if (available_width > 0)
	{
		processShowButton(RS_BUTTON_CAMERA, &available_width);
	}

	if (available_width > 0)
	{
		processShowButton(RS_BUTTON_SNAPSHOT, &available_width);
	}

	processExtendButtons(&available_width);

	// if we have to show/extend some buttons but resized delta width is not enough...
	S32 processed_width = total_available_width - available_width;
	if (processed_width > delta_width)
	{
		// ... let's shrink nearby chat & chiclet panels
		S32 required_to_process_width = processed_width;

		// 1. use delta width of resizing
		required_to_process_width -= delta_width;

		// 2. use width available via decreasing of nearby chat panel
		S32 chatbar_shrink_width = required_to_process_width;
		if (chatbar_available_shrink_width < chatbar_shrink_width)
		{
			chatbar_shrink_width = chatbar_available_shrink_width;
		}

		log(mNearbyChatBar, "increase width: before applying compensative width");
		mNearbyChatBar->reshape(mNearbyChatBar->getRect().getWidth() - chatbar_shrink_width, mNearbyChatBar->getRect().getHeight() );
		if (mNearbyChatBar)			log(mNearbyChatBar, "after applying compensative width");
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
	delta_width -= processed_width;


	// how many space can nearby chatbar take?
	S32 chatbar_panel_width_ = mNearbyChatBar->getRect().getWidth();
	if (delta_width > 0 && chatbar_panel_width_ < chatbar_panel_max_width)
	{
		S32 delta_panel_max = chatbar_panel_max_width - chatbar_panel_width_;
		S32 delta_panel = llmin(delta_width, delta_panel_max);
		lldebugs << "Unprocesed delta width: " << delta_width
			<< ", can be applied to chatbar: " << delta_panel_max
			<< ", will be applied: " << delta_panel
			<< llendl;

		delta_width -= delta_panel_max;
		mNearbyChatBar->reshape(chatbar_panel_width_ + delta_panel, mNearbyChatBar->getRect().getHeight());
		log(mNearbyChatBar, "applied unprocessed delta width");
	}
}

bool LLBottomTray::processShowButton(EResizeState shown_object_type, S32* available_width)
{
	lldebugs << "Trying to show object type: " << shown_object_type << llendl;
	llassert(mStateProcessedObjectMap[shown_object_type] != NULL);

	LLPanel* panel = mStateProcessedObjectMap[shown_object_type];
	if (NULL == panel)
	{
		lldebugs << "There is no object to process for state: " << shown_object_type << llendl;
		return false;
	}
	bool can_be_shown = canButtonBeShown(shown_object_type);
	if (can_be_shown)
	{
		//validate if we have enough room to show this button
		const S32 required_width = panel->getRect().getWidth();
		can_be_shown = *available_width >= required_width;
		if (can_be_shown)
		{
			*available_width -= required_width;

			setTrayButtonVisible(shown_object_type, true);

			lldebugs << "processed object type: " << shown_object_type
				<< ", rest available width: " << *available_width
				<< llendl;
			mResizeState &= ~shown_object_type;
		}
	}
	return can_be_shown;
}

void LLBottomTray::processHideButton(EResizeState processed_object_type, S32* required_width, S32* buttons_freed_width)
{
	lldebugs << "Trying to hide object type: " << processed_object_type << llendl;
	llassert(mStateProcessedObjectMap[processed_object_type] != NULL);

	LLPanel* panel = mStateProcessedObjectMap[processed_object_type];
	if (NULL == panel)
	{
		lldebugs << "There is no object to process for state: " << processed_object_type << llendl;
		return;
	}

	if (panel->getVisible())
	{
		*required_width += panel->getRect().getWidth();

		if (*required_width > 0)
		{
			*buttons_freed_width += *required_width;
		}

		setTrayButtonVisible(processed_object_type, false);

		mResizeState |= processed_object_type;

		lldebugs << "processing object type: " << processed_object_type
			<< ", buttons_freed_width: " << *buttons_freed_width
			<< llendl;
	}
}

void LLBottomTray::processShrinkButtons(S32* required_width, S32* buttons_freed_width)
{
	processShrinkButton(RS_BUTTON_CAMERA, required_width);

	if (*required_width < 0)
	{
		processShrinkButton(RS_BUTTON_MOVEMENT, required_width);
	}
	if (*required_width < 0)
	{
		processShrinkButton(RS_BUTTON_GESTURES, required_width);
	}
	if (*required_width < 0)
	{

		S32 panel_min_width = 0;
		std::string panel_name = mSpeakPanel->getName();
		bool success = mToolbarStack->getPanelMinSize(panel_name, &panel_min_width, NULL);
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

				*required_width += possible_shrink_width;

				if (*required_width > 0)
				{
					*buttons_freed_width += *required_width;
				}

				lldebugs << "Shrunk panel: " << panel_name
					<< ", shrunk width: " << possible_shrink_width
					<< ", rest width to process: " << *required_width
					<< llendl;
			}
		}
	}
}

void LLBottomTray::processShrinkButton(EResizeState processed_object_type, S32* required_width)
{
	llassert(mStateProcessedObjectMap[processed_object_type] != NULL);
	LLPanel* panel = mStateProcessedObjectMap[processed_object_type];
	if (NULL == panel)
	{
		lldebugs << "There is no object to process for type: " << processed_object_type << llendl;
		return;
	}

	if (panel->getVisible())
	{
		S32 panel_width = panel->getRect().getWidth();
		S32 panel_min_width = 0;
		std::string panel_name = panel->getName();
		bool success = mToolbarStack->getPanelMinSize(panel_name, &panel_min_width, NULL);
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
			*required_width += possible_shrink_width;

			// 2. it it is too much... 
			if (*required_width > 0)
			{
				// reduce applied shrunk width to the excessive value.
				possible_shrink_width -= *required_width;
				*required_width = 0;
			}
			panel->reshape(panel_width - possible_shrink_width, panel->getRect().getHeight());

			lldebugs << "Shrunk panel: " << panel_name
				<< ", shrunk width: " << possible_shrink_width
				<< ", rest width to process: " << *required_width
				<< llendl;
		}
	}
}


void LLBottomTray::processExtendButtons(S32* available_width)
{
	// do not allow extending any buttons if we have some buttons hidden
	if (mResizeState & RS_BUTTONS_CAN_BE_HIDDEN) return;

	processExtendButton(RS_BUTTON_GESTURES, available_width);

	if (*available_width > 0)
	{
		processExtendButton(RS_BUTTON_MOVEMENT, available_width);
	}
	if (*available_width > 0)
	{
		processExtendButton(RS_BUTTON_CAMERA, available_width);
	}
	if (*available_width > 0)
	{
		S32 panel_max_width = mObjectDefaultWidthMap[RS_BUTTON_SPEAK];
		S32 panel_width = mSpeakPanel->getRect().getWidth();
		S32 possible_extend_width = panel_max_width - panel_width;
		if (possible_extend_width >= 0 && possible_extend_width <= *available_width)  // HACK: this button doesn't change size so possible_extend_width will be 0
		{
			mSpeakBtn->setLabelVisible(true);
			mSpeakPanel->reshape(panel_max_width, mSpeakPanel->getRect().getHeight());
			log(mSpeakBtn, "speak button is extended");

			*available_width -= possible_extend_width;

			lldebugs << "Extending panel: " << mSpeakPanel->getName()
				<< ", extended width: " << possible_extend_width
				<< ", rest width to process: " << *available_width
				<< llendl;
		}
	}
}

void LLBottomTray::processExtendButton(EResizeState processed_object_type, S32* available_width)
{
	llassert(mStateProcessedObjectMap[processed_object_type] != NULL);
	LLPanel* panel = mStateProcessedObjectMap[processed_object_type];
	if (NULL == panel)
	{
		lldebugs << "There is no object to process for type: " << processed_object_type << llendl;
		return;
	}

	if (!panel->getVisible()) return;

	S32 panel_max_width = mObjectDefaultWidthMap[processed_object_type];
	S32 panel_width = panel->getRect().getWidth();
	S32 possible_extend_width = panel_max_width - panel_width;

	if (possible_extend_width > 0)
	{
		// let calculate real width to extend

		// 1. apply all possible width
		*available_width -= possible_extend_width;

		// 2. it it is too much... 
		if (*available_width < 0)
		{
			// reduce applied extended width to the excessive value.
			possible_extend_width += *available_width;
			*available_width = 0;
		}
		panel->reshape(panel_width + possible_extend_width, panel->getRect().getHeight());

		lldebugs << "Extending panel: " << panel->getName()
			<< ", extended width: " << possible_extend_width
			<< ", rest width to process: " << *available_width
			<< llendl;
	}
}

bool LLBottomTray::canButtonBeShown(EResizeState processed_object_type) const
{
	bool can_be_shown = mResizeState & processed_object_type;
	if (can_be_shown)
	{
		static MASK MOVEMENT_PREVIOUS_BUTTONS_MASK = RS_BUTTON_GESTURES;
		static MASK CAMERA_PREVIOUS_BUTTONS_MASK = RS_BUTTON_GESTURES | RS_BUTTON_MOVEMENT;
		static MASK SNAPSHOT_PREVIOUS_BUTTONS_MASK = RS_BUTTON_GESTURES | RS_BUTTON_MOVEMENT | RS_BUTTON_CAMERA;

		switch(processed_object_type)
		{
		case RS_BUTTON_GESTURES: // Gestures should be shown first
			break;
		case RS_BUTTON_MOVEMENT: // Move only if gesture is shown
			can_be_shown = !(MOVEMENT_PREVIOUS_BUTTONS_MASK & mResizeState);
			break;
		case RS_BUTTON_CAMERA:
			can_be_shown = !(CAMERA_PREVIOUS_BUTTONS_MASK & mResizeState);
			break;
		case RS_BUTTON_SNAPSHOT:
			can_be_shown = !(SNAPSHOT_PREVIOUS_BUTTONS_MASK & mResizeState);
			break;
		default: // nothing to do here
			break;
		}
	}
	return can_be_shown;
}

void LLBottomTray::initStateProcessedObjectMap()
{
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_GESTURES, mGesturePanel));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_MOVEMENT, mMovementPanel));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_CAMERA, mCamPanel));
	mStateProcessedObjectMap.insert(std::make_pair(RS_BUTTON_SNAPSHOT, mSnapshotPanel));

	mDummiesMap.insert(std::make_pair(RS_BUTTON_GESTURES, getChild<LLUICtrl>("after_gesture_panel")));
	mDummiesMap.insert(std::make_pair(RS_BUTTON_MOVEMENT, getChild<LLUICtrl>("after_movement_panel")));
	mDummiesMap.insert(std::make_pair(RS_BUTTON_CAMERA,   getChild<LLUICtrl>("after_cam_panel")));
	mDummiesMap.insert(std::make_pair(RS_BUTTON_SPEAK,    getChild<LLUICtrl>("after_speak_panel")));
}

void LLBottomTray::setTrayButtonVisible(EResizeState shown_object_type, bool visible)
{
	llassert(mStateProcessedObjectMap[shown_object_type] != NULL);
	LLPanel* panel = mStateProcessedObjectMap[shown_object_type];
	if (NULL == panel)
	{
		lldebugs << "There is no object to show for state: " << shown_object_type << llendl;
		return;
	}

	panel->setVisible(visible);

	if (mDummiesMap.count(shown_object_type))
	{
		// Hide/show layout panel for dummy icon.
		mDummiesMap[shown_object_type]->getParent()->setVisible(visible);
	}
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
	LLPanel* cur_panel = mStateProcessedObjectMap[object_type];
	if (NULL == cur_panel)
	{
		lldebugs << "There is no object to process for state: " << object_type << llendl;
		return false;
	}

	const S32 dummy_width = mDummiesMap.count(object_type)
		? mDummiesMap[object_type]->getParent()->getRect().getWidth()
		: 0;

	bool is_set = true;

	if (visible)
	{
		// Assume that only chiclet panel can be auto-resized and
		// don't take into account width of dummy widgets
		const S32 available_width =
			mChicletPanel->getParent()->getRect().getWidth() -
			mChicletPanel->getMinWidth() -
			dummy_width;

		S32 preferred_width = mObjectDefaultWidthMap[object_type];
		S32 current_width = cur_panel->getRect().getWidth();
		S32 result_width = 0;
		bool decrease_width = false;

		// Mark this button to be shown
		mResizeState |= object_type;

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
				mNearbyChatBar->getRect().getWidth() - get_panel_min_width(mToolbarStack, mNearbyChatBar);

			const S32 sum_of_min_widths =
				get_panel_min_width(mToolbarStack, mStateProcessedObjectMap[RS_BUTTON_CAMERA])   +
				get_panel_min_width(mToolbarStack, mStateProcessedObjectMap[RS_BUTTON_MOVEMENT]) +
				get_panel_min_width(mToolbarStack, mStateProcessedObjectMap[RS_BUTTON_GESTURES]) +
				get_panel_min_width(mToolbarStack, mSpeakPanel);

			const S32 sum_of_curr_widths =
				get_curr_width(mStateProcessedObjectMap[RS_BUTTON_CAMERA])   +
				get_curr_width(mStateProcessedObjectMap[RS_BUTTON_MOVEMENT]) +
				get_curr_width(mStateProcessedObjectMap[RS_BUTTON_GESTURES]) +
				get_curr_width(mSpeakPanel);

			const S32 possible_shrunk_width =
				chatbar_shrunk_width + (sum_of_curr_widths - sum_of_min_widths);

			// Minimal width of current panel
			S32 minimal_width = 0;
			mToolbarStack->getPanelMinSize(cur_panel->getName(), &minimal_width, NULL);

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

		is_set = processShowButton(object_type, &current_width);

		// Shrink buttons if needed
		if (is_set && decrease_width)
		{
			processWidthDecreased( -result_width - dummy_width );
		}
	}
	else
	{
		const S32 delta_width = get_curr_width(cur_panel);

		setTrayButtonVisible(object_type, false);

		// Mark button NOT to show while future bottom tray extending
		mResizeState &= ~object_type;

		// Extend other buttons if need
		if (delta_width)
		{
			processWidthIncreased(delta_width + dummy_width);
		}
	}
	return is_set;
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

//EOF
