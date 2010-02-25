/** 
 * @file llmoveview.cpp
 * @brief Container for movement buttons like forward, left, fly
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llmoveview.h"

// Library includes
#include "indra_constants.h"
#include "llparcel.h"

// Viewer includes

#include "llagent.h"
#include "llvoavatarself.h" // to check gAgent.getAvatarObject()->isSitting()
#include "llbottomtray.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "lljoystickbutton.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llselectmgr.h" 
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "lltooltip.h"

//
// Constants
//

const F32 MOVE_BUTTON_DELAY = 0.0f;
const F32 YAW_NUDGE_RATE = 0.05f;	// fraction of normal speed
const F32 NUDGE_TIME = 0.25f;		// in seconds

const std::string BOTTOM_TRAY_BUTTON_NAME = "movement_btn";

//
// Member functions
//

// protected
LLFloaterMove::LLFloaterMove(const LLSD& key)
:	LLTransientDockableFloater(NULL, true, key),
	mForwardButton(NULL),
	mBackwardButton(NULL),
	mTurnLeftButton(NULL), 
	mTurnRightButton(NULL),
	mMoveUpButton(NULL),
	mMoveDownButton(NULL),
	mStopFlyingButton(NULL),
	mModeActionsPanel(NULL),
	mCurrentMode(MM_WALK)
{
}

// virtual
BOOL LLFloaterMove::postBuild()
{
	setIsChrome(TRUE);
	setTitleVisible(TRUE); // restore title visibility after chrome applying
	
	LLDockableFloater::postBuild();
	
	mForwardButton = getChild<LLJoystickAgentTurn>("forward btn"); 
	mForwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mBackwardButton = getChild<LLJoystickAgentTurn>("backward btn"); 
	mBackwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

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


	mStopFlyingButton = getChild<LLButton>("stop_fly_btn");

	mModeActionsPanel = getChild<LLPanel>("panel_modes");

	LLButton* btn;
	btn = getChild<LLButton>("mode_walk_btn");
	btn->setCommitCallback(boost::bind(&LLFloaterMove::onWalkButtonClick, this));

	btn = getChild<LLButton>("mode_run_btn");
	btn->setCommitCallback(boost::bind(&LLFloaterMove::onRunButtonClick, this));

	btn = getChild<LLButton>("mode_fly_btn");
	btn->setCommitCallback(boost::bind(&LLFloaterMove::onFlyButtonClick, this));

	btn = getChild<LLButton>("stop_fly_btn");
	btn->setCommitCallback(boost::bind(&LLFloaterMove::onStopFlyingButtonClick, this));



	showFlyControls(false);

	initModeTooltips();

	initModeButtonMap();

	initMovementMode();

	LLViewerParcelMgr::getInstance()->addAgentParcelChangedCallback(LLFloaterMove::sUpdateFlyingStatus);

	return TRUE;
}

// virtual
void LLFloaterMove::setEnabled(BOOL enabled)
{
	//we need to enable/disable only buttons, EXT-1061.

	// is called before postBuild() - use findChild here.
	LLPanel *panel_actions = findChild<LLPanel>("panel_actions");
	if (panel_actions) panel_actions->setEnabled(enabled);

	showModeButtons(enabled);
}

// *NOTE: we assume that setVisible() is called on floater close.
// virtual
void LLFloaterMove::setVisible(BOOL visible)
{
	// Do nothing with Stand/Stop Flying panel in excessive calls of this method (from LLTransientFloaterMgr?).
	if (getVisible() == visible)
	{
		LLTransientDockableFloater::setVisible(visible);
		return;
	}

	if (visible)
	{
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

	LLTransientDockableFloater::setVisible(visible);
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
		instance->showModeButtons(!fly);
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
void LLFloaterMove::onStopFlyingButtonClick()
{
	setMovementMode(gAgent.getAlwaysRun() ? MM_RUN : MM_WALK);
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
		|| (gAgent.getAvatarObject() && gAgent.getAvatarObject()->isSitting());

	showModeButtons(!bHideModeButtons);

}

void LLFloaterMove::updateButtonsWithMovementMode(const EMovementMode newMode)
{
	showFlyControls(MM_FLY == newMode);
	setModeTooltip(newMode);
	setModeButtonToggleState(newMode);
	setModeTitle(newMode);
}

void LLFloaterMove::showFlyControls(bool bShow)
{
	mMoveUpButton->setVisible(bShow);
	mMoveDownButton->setVisible(bShow);

	// *TODO: mantipov: mStopFlyingButton from the FloaterMove is not used now.
	// It was not completly removed until functionality is reviewed by LL
	mStopFlyingButton->setVisible(FALSE);
}

void LLFloaterMove::initModeTooltips()
{
	control_tooltip_map_t walkTipMap;
	walkTipMap.insert(std::make_pair(mForwardButton, getString("walk_forward_tooltip")));
	walkTipMap.insert(std::make_pair(mBackwardButton, getString("walk_back_tooltip")));
	mModeControlTooltipsMap[MM_WALK] = walkTipMap;

	control_tooltip_map_t runTipMap;
	runTipMap.insert(std::make_pair(mForwardButton, getString("run_forward_tooltip")));
	runTipMap.insert(std::make_pair(mBackwardButton, getString("run_back_tooltip")));
	mModeControlTooltipsMap[MM_RUN] = runTipMap;

	control_tooltip_map_t flyTipMap;
	flyTipMap.insert(std::make_pair(mForwardButton, getString("fly_forward_tooltip")));
	flyTipMap.insert(std::make_pair(mBackwardButton, getString("fly_back_tooltip")));
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

	if (gAgent.getAvatarObject())
	{
		setEnabled(!gAgent.getAvatarObject()->isSitting());
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

/**
 * Updates position of the floater to be center aligned with Move button.
 */
void LLFloaterMove::updatePosition()
{
	LLBottomTray* tray = LLBottomTray::getInstance();
	if (!tray) return;

	LLButton* movement_btn = tray->getChild<LLButton>(BOTTOM_TRAY_BUTTON_NAME);

	//align centers of a button and a floater
	S32 x = movement_btn->calcScreenRect().getCenterX() - getRect().getWidth()/2;

	S32 y = 0;
	if (!mModeActionsPanel->getVisible())
	{
		y = mModeActionsPanel->getRect().getHeight();
	}
	setOrigin(x, y);
}

//static
void LLFloaterMove::sUpdateFlyingStatus()
{
	LLFloaterMove *floater = LLFloaterReg::findTypedInstance<LLFloaterMove>("moveview");
	if (floater) floater->mModeControlButtonMap[MM_FLY]->setEnabled(gAgent.canFly());
	
}

void LLFloaterMove::showModeButtons(BOOL bShow)
{
	// is called from setEnabled so can be called before postBuild(), check mModeActionsPanel agains to NULL
	if (NULL == mModeActionsPanel || mModeActionsPanel->getVisible() == bShow)
		return;
	mModeActionsPanel->setVisible(bShow);
}

//static
void LLFloaterMove::enableInstance(BOOL bEnable)
{
	LLFloaterMove* instance = LLFloaterReg::findTypedInstance<LLFloaterMove>("moveview");
	if (instance)
	{
		instance->setEnabled(bEnable);

		if (gAgent.getFlying())
		{
			instance->showModeButtons(FALSE);
		}
	}
}

void LLFloaterMove::onOpen(const LLSD& key)
{
	LLButton *anchor_panel = LLBottomTray::getInstance()->getChild<LLButton>("movement_btn");

	if (gAgent.getFlying())
	{
		setFlyingMode(TRUE);
		showModeButtons(FALSE);
	}

	if (gAgent.getAvatarObject() && gAgent.getAvatarObject()->isSitting())
	{
		setSittingMode(TRUE);
		showModeButtons(FALSE);
	}

	setDockControl(new LLDockControl(
		anchor_panel, this,
		getDockTongue(), LLDockControl::TOP));

	sUpdateFlyingStatus();
}

//virtual
void LLFloaterMove::setDocked(bool docked, bool pop_on_undock/* = true*/)
{
	LLTransientDockableFloater::setDocked(docked, pop_on_undock);
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
inline LLPanelStandStopFlying* LLPanelStandStopFlying::getInstance()
{
	static LLPanelStandStopFlying* panel = getStandStopFlyingPanel();
	return panel;
}

//static
void LLPanelStandStopFlying::setStandStopFlyingMode(EStandStopFlyingMode mode)
{
	LLPanelStandStopFlying* panel = getInstance();

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
	if (gAgent.getCameraMode() == CAMERA_MODE_MOUSELOOK) visible = false;

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
	LLUICtrlFactory::getInstance()->buildPanel(panel, "panel_stand_stop_flying.xml");

	panel->setVisible(FALSE);
	//LLUI::getRootView()->addChild(panel);

	llinfos << "Build LLPanelStandStopFlying panel" << llendl;

	panel->updatePosition();
	return panel;
}

void LLPanelStandStopFlying::onStandButtonClick()
{
	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);

	setFocus(FALSE); // EXT-482
}

void LLPanelStandStopFlying::onStopFlyingButtonClick()
{
	gAgent.setFlying(FALSE);

	setFocus(FALSE); // EXT-482
	setVisible(FALSE);
}

/**
 * Updates position of the Stand & Stop Flying panel to be center aligned with Move button.
 */
void LLPanelStandStopFlying::updatePosition()
{
	LLBottomTray* tray = LLBottomTray::getInstance();
	if (!tray || mAttached) return;

	LLButton* movement_btn = tray->getChild<LLButton>(BOTTOM_TRAY_BUTTON_NAME);

	// Align centers of the button and the panel.
	S32 x = movement_btn->calcScreenRect().getCenterX() - getRect().getWidth()/2;
	setOrigin(x, 0);
}


// EOF
