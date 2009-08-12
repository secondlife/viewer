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

// Viewer includes

#include "llagent.h"
#include "llvoavatarself.h" // to check gAgent.getAvatarObject()->isSitting()
#include "llbottomtray.h"
#include "llbutton.h"
#include "llfirsttimetipmanager.h"
#include "llfloaterreg.h"
#include "llfloaterfirsttimetip.h"
#include "lljoystickbutton.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llselectmgr.h" 

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
:	LLFloater(key),
	mForwardButton(NULL),
	mBackwardButton(NULL),
	mTurnLeftButton(NULL), 
	mTurnRightButton(NULL),
	mMoveUpButton(NULL),
	mMoveDownButton(NULL),
	mStopFlyingButton(NULL),
	mModeActionsPanel(NULL)
{
}

// virtual
BOOL LLFloaterMove::postBuild()
{
	setIsChrome(TRUE);
	
	
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

	updatePosition();

	initModeButtonMap();

	initMovementMode();

	return TRUE;
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
	gAgent.setFlying(MM_FLY == mode);

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

	showQuickTips(mode);
}

void LLFloaterMove::updateButtonsWithMovementMode(const EMovementMode newMode)
{
	showFlyControls(MM_FLY == newMode);
	setModeTooltip(newMode);
	setModeButtonToggleState(newMode);
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

/**
 * Updates position of the floater to be center aligned with Move button.
 * 
 * Because Tip floater created as dependent floater this method 
 * must be called before "showQuickTips()" to get Tip floater be positioned at the right side of the floater
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
void LLFloaterMove::showModeButtons(BOOL bShow)
{
	if (mModeActionsPanel->getVisible() == bShow)
		return;
	mModeActionsPanel->setVisible(bShow);

	LLRect rect = getRect();

	static S32 height = mModeActionsPanel->getRect().getHeight();
	S32 newHeight = getRect().getHeight();
	if (!bShow)
	{
		newHeight -= height;
	}
	else
	{
		newHeight += height;
	}
	rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.getWidth(), newHeight);
	reshape(rect.getWidth(), rect.getHeight());
	setRect(rect);
}
//static
void LLFloaterMove::enableInstance(BOOL bEnable)
{
	LLFloaterMove* instance = LLFloaterReg::findTypedInstance<LLFloaterMove>("moveview");
	if (instance)
	{
		instance->setEnabled(bEnable);
		instance->showModeButtons(bEnable);
	}
}

void LLFloaterMove::onOpen(const LLSD& key)
{
	updatePosition();
}

void LLFloaterMove::showQuickTips(const EMovementMode mode)
{
	LLFirstTimeTipsManager::EFirstTimeTipType tipType = LLFirstTimeTipsManager::FTT_MOVE_WALK;
	switch (mode)
	{
	case MM_FLY:	tipType = LLFirstTimeTipsManager::FTT_MOVE_FLY;			break;
	case MM_RUN:	tipType = LLFirstTimeTipsManager::FTT_MOVE_RUN;			break;
	case MM_WALK:	tipType = LLFirstTimeTipsManager::FTT_MOVE_WALK;		break;
	default: llwarns << "Quick Tip type was not detected, FTT_MOVE_WALK will be used" << llendl;
	}

	LLFirstTimeTipsManager::showTipsFor(tipType, this, LLFirstTimeTipsManager::TPA_POS_LEFT_ALIGN_TOP);
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
	mStopFlyingButton(NULL)
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
	panel->setVisible(TRUE);

	BOOL standVisible = SSFM_STAND == mode;
	panel->mStandButton->setVisible(standVisible);
	panel->mStopFlyingButton->setVisible(!standVisible);
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
	
	mStopFlyingButton = getChild<LLButton>("stop_fly_btn");
	mStopFlyingButton->setCommitCallback(boost::bind(&LLFloaterMove::setFlyingMode, FALSE));
	mStopFlyingButton->setCommitCallback(boost::bind(&LLPanelStandStopFlying::onStopFlyingButtonClick, this));

	
	return TRUE;
}

//virtual
void LLPanelStandStopFlying::setVisible(BOOL visible)
{
	if (visible)
	{
		updatePosition();
	}

	LLPanel::setVisible(visible);
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
	LLUI::getRootView()->addChild(panel);

	llinfos << "Build LLPanelStandStopFlying panel" << llendl;

	panel->updatePosition();
	return panel;
}

void LLPanelStandStopFlying::onStandButtonClick()
{
	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);

	setVisible(FALSE);
}

void LLPanelStandStopFlying::onStopFlyingButtonClick()
{
	gAgent.setFlying(FALSE);

	setVisible(FALSE);
}

/**
 * Updates position of the Stand & Stop Flying panel to be center aligned with Move button.
 */
void LLPanelStandStopFlying::updatePosition()
{

	LLBottomTray* tray = LLBottomTray::getInstance();
	if (!tray) return;

	LLButton* movement_btn = tray->getChild<LLButton>(BOTTOM_TRAY_BUTTON_NAME);

	//align centers of a button and a floater
	S32 x = movement_btn->calcScreenRect().getCenterX() - getRect().getWidth()/2;

	S32 y = tray->getRect().getHeight();

	setOrigin(x, y);
}


// EOF
