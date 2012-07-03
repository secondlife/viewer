/** 
 * @file llmoveview.cpp
 * @brief Container for movement buttons like forward, left, fly
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

#include "llmoveview.h"

// Library includes
#include "indra_constants.h"
#include "llparcel.h"

// Viewer includes

#include "llagent.h"
#include "llagentcamera.h"
#include "llvoavatarself.h" // to check gAgentAvatarp->isSitting()
#include "llbutton.h"
#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llhints.h"
#include "lljoystickbutton.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llselectmgr.h"
#include "lltoolbarview.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "lltooltip.h"

//
// Constants
//

const F32 MOVE_BUTTON_DELAY = 0.0f;
const F32 YAW_NUDGE_RATE = 0.05f;	// fraction of normal speed
const F32 NUDGE_TIME = 0.25f;		// in seconds

//
// Member functions
//

// protected
LLFloaterMove::LLFloaterMove(const LLSD& key)
:	LLFloater(key),
	mForwardButton(NULL),
	mBackwardButton(NULL),
	mTurnLeftButton(NULL), 
	mTurnRightButton(NULL),
	mMoveUpButton(NULL),
	mMoveDownButton(NULL),
	mModeActionsPanel(NULL),
	mCurrentMode(MM_WALK)
{
}

LLFloaterMove::~LLFloaterMove()
{
	// Ensure LLPanelStandStopFlying panel is not among floater's children. See EXT-8458.
	setVisible(FALSE);

	// Otherwise it can be destroyed and static pointer in LLPanelStandStopFlying::getInstance() will become invalid.
	// Such situation was possible when LLFloaterReg returns "dead" instance of floater.
	// Should not happen after LLFloater::destroy was modified to remove "dead" instances from LLFloaterReg.
}

// virtual
BOOL LLFloaterMove::postBuild()
{
	updateTransparency(TT_ACTIVE); // force using active floater transparency (STORM-730)
	
	// Code that implements floater buttons toggling when user moves via keyboard is located in LLAgent::propagate()

	mForwardButton = getChild<LLJoystickAgentTurn>("forward btn"); 
	mForwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mBackwardButton = getChild<LLJoystickAgentTurn>("backward btn"); 
	mBackwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mSlideLeftButton = getChild<LLJoystickAgentSlide>("move left btn");
	mSlideLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mSlideRightButton = getChild<LLJoystickAgentSlide>("move right btn");
	mSlideRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mTurnLeftButton = getChild<LLButton>("turn left btn"); 
	mTurnLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnLeftButton->setHeldDownCallback(boost::bind(&LLFloaterMove::turnLeft, this));
	mTurnRightButton = getChild<LLButton>("turn right btn"); 
	mTurnRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnRightButton->setHeldDownCallback(boost::bind(&LLFloaterMove::turnRight, this));

	mMoveUpButton = getChild<LLButton>("move up btn"); 
	mMoveUpButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveUpButton->setHeldDownCallback(boost::bind(&LLFloaterMove::moveUp, this));

	mMoveDownButton = getChild<LLButton>("move down btn"); 
	mMoveDownButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveDownButton->setHeldDownCallback(boost::bind(&LLFloaterMove::moveDown, this));


	mModeActionsPanel = getChild<LLPanel>("panel_modes");

	LLButton* btn;
	btn = getChild<LLButton>("mode_walk_btn");
	btn->setCommitCallback(boost::bind(&LLFloaterMove::onWalkButtonClick, this));

	btn = getChild<LLButton>("mode_run_btn");
	btn->setCommitCallback(boost::bind(&LLFloaterMove::onRunButtonClick, this));

	btn = getChild<LLButton>("mode_fly_btn");
	btn->setCommitCallback(boost::bind(&LLFloaterMove::onFlyButtonClick, this));

	initModeTooltips();

	initModeButtonMap();

	initMovementMode();

	LLViewerParcelMgr::getInstance()->addAgentParcelChangedCallback(LLFloaterMove::sUpdateFlyingStatus);

	return TRUE;
}

// *NOTE: we assume that setVisible() is called on floater close.
// virtual
void LLFloaterMove::setVisible(BOOL visible)
{
	// Do nothing with Stand/Stop Flying panel in excessive calls of this method.
	if (getVisible() == visible)
	{
		LLFloater::setVisible(visible);
		return;
	}

	if (visible)
	{
		LLFirstUse::notMoving(false);
		// Attach the Stand/Stop Flying panel.
		LLPanelStandStopFlying* ssf_panel = LLPanelStandStopFlying::getInstance();
		ssf_panel->reparent(this);
		const LLRect& mode_actions_rect = mModeActionsPanel->getRect();
		ssf_panel->setOrigin(mode_actions_rect.mLeft, mode_actions_rect.mBottom);
	}
	else
	{
		// Detach the Stand/Stop Flying panel.
		LLPanelStandStopFlying::getInstance()->reparent(NULL);
	}

	LLFloater::setVisible(visible);
}

// static 
F32 LLFloaterMove::getYawRate( F32 time )
{
	if( time < NUDGE_TIME )
	{
		F32 rate = YAW_NUDGE_RATE + time * (1 - YAW_NUDGE_RATE)/ NUDGE_TIME;
		return rate;
	}
	else
	{
		return 1.f;
	}
}


// static 
void LLFloaterMove::setFlyingMode(BOOL fly)
{
	LLFloaterMove* instance = LLFloaterReg::findTypedInstance<LLFloaterMove>("moveview");
	if (instance)
	{
		instance->setFlyingModeImpl(fly);
		LLVOAvatarSelf* avatar_object = gAgentAvatarp;
		bool is_sitting = avatar_object
			&& (avatar_object->getRegion() != NULL)
			&& (!avatar_object->isDead())
			&& avatar_object->isSitting();
		instance->showModeButtons(!fly && !is_sitting);
	}
	if (fly)
	{
		LLPanelStandStopFlying::setStandStopFlyingMode(LLPanelStandStopFlying::SSFM_STOP_FLYING);
	}
	else
	{
		LLPanelStandStopFlying::clearStandStopFlyingMode(LLPanelStandStopFlying::SSFM_STOP_FLYING);
	}
}
//static
void LLFloaterMove::setAlwaysRunMode(bool run)
{
	LLFloaterMove* instance = LLFloaterReg::findTypedInstance<LLFloaterMove>("moveview");
	if (instance)
	{
		instance->setAlwaysRunModeImpl(run);
	}
}

void LLFloaterMove::setFlyingModeImpl(BOOL fly)
{
	updateButtonsWithMovementMode(fly ? MM_FLY : (gAgent.getAlwaysRun() ? MM_RUN : MM_WALK));
}

void LLFloaterMove::setAlwaysRunModeImpl(bool run)
{
	if (!gAgent.getFlying())
	{
		updateButtonsWithMovementMode(run ? MM_RUN : MM_WALK);
	}
}

//static
void LLFloaterMove::setSittingMode(BOOL bSitting)
{
	if (bSitting)
	{
		LLPanelStandStopFlying::setStandStopFlyingMode(LLPanelStandStopFlying::SSFM_STAND);
	}
	else
	{
		LLPanelStandStopFlying::clearStandStopFlyingMode(LLPanelStandStopFlying::SSFM_STAND);

		// show "Stop Flying" button if needed. EXT-871
		if (gAgent.getFlying())
		{
			LLPanelStandStopFlying::setStandStopFlyingMode(LLPanelStandStopFlying::SSFM_STOP_FLYING);
		}
	}
	enableInstance(!bSitting);
}

// protected 
void LLFloaterMove::turnLeft()
{
	F32 time = mTurnLeftButton->getHeldDownTime();
	gAgent.moveYaw( getYawRate( time ) );
}

// protected
void LLFloaterMove::turnRight()
{
	F32 time = mTurnRightButton->getHeldDownTime();
	gAgent.moveYaw( -getYawRate( time ) );
}

// protected
void LLFloaterMove::moveUp()
{
	// Jumps or flys up, depending on fly state
	gAgent.moveUp(1);
}

// protected
void LLFloaterMove::moveDown()
{
	// Crouches or flys down, depending on fly state
	gAgent.moveUp(-1);
}

//////////////////////////////////////////////////////////////////////////
// Private Section:
//////////////////////////////////////////////////////////////////////////

void LLFloaterMove::onWalkButtonClick()
{
	setMovementMode(MM_WALK);
}
void LLFloaterMove::onRunButtonClick()
{
	setMovementMode(MM_RUN);
}
void LLFloaterMove::onFlyButtonClick()
{
	setMovementMode(MM_FLY);
}

void LLFloaterMove::setMovementMode(const EMovementMode mode)
{
	mCurrentMode = mode;
	gAgent.setFlying(MM_FLY == mode);

	// attempts to set avatar flying can not set it real flying in some cases.
	// For ex. when avatar fell down & is standing up.
	// So, no need to continue processing FLY mode. See EXT-1079
	if (MM_FLY == mode && !gAgent.getFlying())
	{
		return;
	}

	switch (mode)
	{
	case MM_RUN:
		gAgent.setAlwaysRun();
		gAgent.setRunning();
		break;
	case MM_WALK:
		gAgent.clearAlwaysRun();
		gAgent.clearRunning();
		break;
	default:
		//do nothing for other modes (MM_FLY)
		break;
	}
	// tell the simulator.
	gAgent.sendWalkRun(gAgent.getAlwaysRun());
	
	updateButtonsWithMovementMode(mode);

	bool bHideModeButtons = MM_FLY == mode
		|| (isAgentAvatarValid() && gAgentAvatarp->isSitting());

	showModeButtons(!bHideModeButtons);

}

void LLFloaterMove::updateButtonsWithMovementMode(const EMovementMode newMode)
{
	setModeTooltip(newMode);
	setModeButtonToggleState(newMode);
	setModeTitle(newMode);
}

void LLFloaterMove::initModeTooltips()
{
	control_tooltip_map_t walkTipMap;
	walkTipMap.insert(std::make_pair(mForwardButton, getString("walk_forward_tooltip")));
	walkTipMap.insert(std::make_pair(mBackwardButton, getString("walk_back_tooltip")));
	walkTipMap.insert(std::make_pair(mSlideLeftButton, getString("walk_left_tooltip")));
	walkTipMap.insert(std::make_pair(mSlideRightButton, getString("walk_right_tooltip")));
	walkTipMap.insert(std::make_pair(mMoveUpButton, getString("jump_tooltip")));
	walkTipMap.insert(std::make_pair(mMoveDownButton, getString("crouch_tooltip")));
	mModeControlTooltipsMap[MM_WALK] = walkTipMap;

	control_tooltip_map_t runTipMap;
	runTipMap.insert(std::make_pair(mForwardButton, getString("run_forward_tooltip")));
	runTipMap.insert(std::make_pair(mBackwardButton, getString("run_back_tooltip")));
	runTipMap.insert(std::make_pair(mSlideLeftButton, getString("run_left_tooltip")));
	runTipMap.insert(std::make_pair(mSlideRightButton, getString("run_right_tooltip")));
	runTipMap.insert(std::make_pair(mMoveUpButton, getString("jump_tooltip")));
	runTipMap.insert(std::make_pair(mMoveDownButton, getString("crouch_tooltip")));
	mModeControlTooltipsMap[MM_RUN] = runTipMap;

	control_tooltip_map_t flyTipMap;
	flyTipMap.insert(std::make_pair(mForwardButton, getString("fly_forward_tooltip")));
	flyTipMap.insert(std::make_pair(mBackwardButton, getString("fly_back_tooltip")));
	flyTipMap.insert(std::make_pair(mSlideLeftButton, getString("fly_left_tooltip")));
	flyTipMap.insert(std::make_pair(mSlideRightButton, getString("fly_right_tooltip")));
	flyTipMap.insert(std::make_pair(mMoveUpButton, getString("fly_up_tooltip")));
	flyTipMap.insert(std::make_pair(mMoveDownButton, getString("fly_down_tooltip")));
	mModeControlTooltipsMap[MM_FLY] = flyTipMap;

	setModeTooltip(MM_WALK);
}

void LLFloaterMove::initModeButtonMap()
{
	mModeControlButtonMap[MM_WALK] = getChild<LLButton>("mode_walk_btn");
	mModeControlButtonMap[MM_RUN] = getChild<LLButton>("mode_run_btn");
	mModeControlButtonMap[MM_FLY] = getChild<LLButton>("mode_fly_btn");
}

void LLFloaterMove::initMovementMode()
{
	EMovementMode initMovementMode = gAgent.getAlwaysRun() ? MM_RUN : MM_WALK;
	if (gAgent.getFlying())
	{
		initMovementMode = MM_FLY;
	}
	setMovementMode(initMovementMode);

	if (isAgentAvatarValid())
	{
		showModeButtons(!gAgentAvatarp->isSitting());
	}
}

void LLFloaterMove::setModeTooltip(const EMovementMode mode)
{
	llassert_always(mModeControlTooltipsMap.end() != mModeControlTooltipsMap.find(mode));
	control_tooltip_map_t controlsTipMap = mModeControlTooltipsMap[mode];
	control_tooltip_map_t::const_iterator it = controlsTipMap.begin();
	for (; it != controlsTipMap.end(); ++it)
	{
		LLView* ctrl = it->first;
		std::string tooltip = it->second;
		ctrl->setToolTip(tooltip);
	}
}

void LLFloaterMove::setModeTitle(const EMovementMode mode)
{
	std::string title; 
	switch(mode)
	{
	case MM_WALK:
		title = getString("walk_title");
		break;
	case MM_RUN:
		title = getString("run_title");
		break;
	case MM_FLY:
		title = getString("fly_title");
		break;
	default:
		// title should be provided for all modes
		llassert(false);
		break;
	}
	setTitle(title);
}

//static
void LLFloaterMove::sUpdateFlyingStatus()
{
	LLFloaterMove *floater = LLFloaterReg::findTypedInstance<LLFloaterMove>("moveview");
	if (floater) floater->mModeControlButtonMap[MM_FLY]->setEnabled(gAgent.canFly());
	
}

void LLFloaterMove::showModeButtons(BOOL bShow)
{
	if (mModeActionsPanel->getVisible() == bShow)
		return;
	mModeActionsPanel->setVisible(bShow);
}

//static
void LLFloaterMove::enableInstance(BOOL bEnable)
{
	LLFloaterMove* instance = LLFloaterReg::findTypedInstance<LLFloaterMove>("moveview");
	if (instance)
	{
		if (gAgent.getFlying())
		{
			instance->showModeButtons(FALSE);
		}
		else
		{
			instance->showModeButtons(bEnable);
		}
	}
}

void LLFloaterMove::onOpen(const LLSD& key)
{
	if (gAgent.getFlying())
	{
		setFlyingMode(TRUE);
		showModeButtons(FALSE);
	}

	if (isAgentAvatarValid() && gAgentAvatarp->isSitting())
	{
		setSittingMode(TRUE);
		showModeButtons(FALSE);
	}

	sUpdateFlyingStatus();
}

void LLFloaterMove::setModeButtonToggleState(const EMovementMode mode)
{
	llassert_always(mModeControlButtonMap.end() != mModeControlButtonMap.find(mode));

	mode_control_button_map_t::const_iterator it = mModeControlButtonMap.begin();
	for (; it != mModeControlButtonMap.end(); ++it)
	{
		it->second->setToggleState(FALSE);
	}

	mModeControlButtonMap[mode]->setToggleState(TRUE);
}



/************************************************************************/
/*                        LLPanelStandStopFlying                        */
/************************************************************************/
LLPanelStandStopFlying::LLPanelStandStopFlying() :
	mStandButton(NULL),
	mStopFlyingButton(NULL),
	mAttached(false)
{
	// make sure we have the only instance of this class
	static bool b = true;
	llassert_always(b);
	b=false;
}

// static
LLPanelStandStopFlying* LLPanelStandStopFlying::getInstance()
{
	static LLPanelStandStopFlying* panel = getStandStopFlyingPanel();
	return panel;
}

//static
void LLPanelStandStopFlying::setStandStopFlyingMode(EStandStopFlyingMode mode)
{
	LLPanelStandStopFlying* panel = getInstance();

	if (mode == SSFM_STAND)
	{
		LLFirstUse::sit();
		LLFirstUse::notMoving(false);
	}
	panel->mStandButton->setVisible(SSFM_STAND == mode);
	panel->mStopFlyingButton->setVisible(SSFM_STOP_FLYING == mode);

	//visibility of it should be updated after updating visibility of the buttons
	panel->setVisible(TRUE);
}

//static
void LLPanelStandStopFlying::clearStandStopFlyingMode(EStandStopFlyingMode mode)
{
	LLPanelStandStopFlying* panel = getInstance();
	switch(mode) {
	case SSFM_STAND:
		panel->mStandButton->setVisible(FALSE);
		break;
	case SSFM_STOP_FLYING:
		panel->mStopFlyingButton->setVisible(FALSE);
		break;
	default:
		llerrs << "Unexpected EStandStopFlyingMode is passed: " << mode << llendl;
	}

}

BOOL LLPanelStandStopFlying::postBuild()
{
	mStandButton = getChild<LLButton>("stand_btn");
	mStandButton->setCommitCallback(boost::bind(&LLPanelStandStopFlying::onStandButtonClick, this));
	mStandButton->setCommitCallback(boost::bind(&LLFloaterMove::enableInstance, TRUE));
	mStandButton->setVisible(FALSE);
	LLHints::registerHintTarget("stand_btn", mStandButton->getHandle());
	
	mStopFlyingButton = getChild<LLButton>("stop_fly_btn");
	//mStopFlyingButton->setCommitCallback(boost::bind(&LLFloaterMove::setFlyingMode, FALSE));
	mStopFlyingButton->setCommitCallback(boost::bind(&LLPanelStandStopFlying::onStopFlyingButtonClick, this));
	mStopFlyingButton->setVisible(FALSE);
	
	return TRUE;
}

//virtual
void LLPanelStandStopFlying::setVisible(BOOL visible)
{
	//we dont need to show the panel if these buttons are not activated
	if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK) visible = false;

	if (visible)
	{
		updatePosition();
	}

	// do not change parent visibility in case panel is attached into Move Floater: EXT-3632, EXT-4646
	if (!mAttached) 
	{
		//change visibility of parent layout_panel to animate in/out. EXT-2504
		if (getParent()) getParent()->setVisible(visible);
	}

	// also change own visibility to avoid displaying the panel in mouselook (broken when EXT-2504 was implemented).
	// See EXT-4718.
	LLPanel::setVisible(visible);
}

BOOL LLPanelStandStopFlying::handleToolTip(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().unblockToolTips();

	if (mStandButton->getVisible())
	{
		LLToolTipMgr::instance().show(mStandButton->getToolTip());
	}
	else if (mStopFlyingButton->getVisible())
	{
		LLToolTipMgr::instance().show(mStopFlyingButton->getToolTip());
	}

	return LLPanel::handleToolTip(x, y, mask);
}

void LLPanelStandStopFlying::reparent(LLFloaterMove* move_view)
{
	LLPanel* parent = dynamic_cast<LLPanel*>(getParent());
	if (!parent)
	{
		llwarns << "Stand/stop flying panel parent is unset, already attached?: " << mAttached << ", new parent: " << (move_view == NULL ? "NULL" : "Move Floater") << llendl;
		return;
	}

	if (move_view != NULL)
	{
		llassert(move_view != parent); // sanity check
	
		// Save our original container.
		if (!mOriginalParent.get())
			mOriginalParent = parent->getHandle();

		// Attach to movement controls.
		parent->removeChild(this);
		move_view->addChild(this);
		// Origin must be set by movement controls.
		mAttached = true;
	}
	else
	{
		if (!mOriginalParent.get())
		{
			llwarns << "Original parent of the stand / stop flying panel not found" << llendl;
			return;
		}

		// Detach from movement controls. 
		parent->removeChild(this);
		mOriginalParent.get()->addChild(this);
		// update parent with self visibility (it is changed in setVisible()). EXT-4743
		mOriginalParent.get()->setVisible(getVisible());

		mAttached = false;
		updatePosition(); // don't defer until next draw() to avoid flicker
	}
}

//////////////////////////////////////////////////////////////////////////
// Private Section
//////////////////////////////////////////////////////////////////////////

//static
LLPanelStandStopFlying* LLPanelStandStopFlying::getStandStopFlyingPanel()
{
	LLPanelStandStopFlying* panel = new LLPanelStandStopFlying();
	panel->buildFromFile("panel_stand_stop_flying.xml");

	panel->setVisible(FALSE);
	//LLUI::getRootView()->addChild(panel);

	llinfos << "Build LLPanelStandStopFlying panel" << llendl;

	panel->updatePosition();
	return panel;
}

void LLPanelStandStopFlying::onStandButtonClick()
{
	LLFirstUse::sit(false);

	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);

	setFocus(FALSE); // EXT-482
	mStandButton->setVisible(FALSE); // force visibility changing to avoid seeing Stand & Move buttons at once.
}

void LLPanelStandStopFlying::onStopFlyingButtonClick()
{
	gAgent.setFlying(FALSE);

	setFocus(FALSE); // EXT-482
	mStopFlyingButton->setVisible(FALSE);
}

/**
 * Updates position of the Stand & Stop Flying panel to be center aligned with Move button.
 */
void LLPanelStandStopFlying::updatePosition()
{
	if (mAttached) return;

	S32 y_pos = 0;
	S32 bottom_tb_center = 0;
	if (LLToolBar* toolbar_bottom = gToolBarView->getChild<LLToolBar>("toolbar_bottom"))
	{
		y_pos = toolbar_bottom->getRect().getHeight();
		bottom_tb_center = toolbar_bottom->getRect().getCenterX();
	}

	S32 left_tb_width = 0;
	if (LLToolBar* toolbar_left = gToolBarView->getChild<LLToolBar>("toolbar_left"))
	{
		left_tb_width = toolbar_left->getRect().getWidth();
	}

	if(LLPanel* panel_ssf_container = getRootView()->getChild<LLPanel>("state_management_buttons_container"))
	{
		panel_ssf_container->setOrigin(0, y_pos);
	}

	S32 x_pos = bottom_tb_center-getRect().getWidth()/2 - left_tb_width;

	setOrigin( x_pos, 0);
}

// EOF
