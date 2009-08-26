/** 
 * @file llmoveview.h
 * @brief Container for buttons for walking, turning, flying
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

#ifndef LL_LLMOVEVIEW_H
#define LL_LLMOVEVIEW_H

// Library includes
#include "llfloater.h"

class LLButton;
class LLJoystickAgentTurn;
class LLJoystickAgentSlide;

//
// Classes
//
class LLFloaterMove
:	public LLFloater
{
	friend class LLFloaterReg;

private:
	LLFloaterMove(const LLSD& key);
	~LLFloaterMove() {}
public:

	/*virtual*/	BOOL	postBuild();
	static F32	getYawRate(F32 time);
	static void setFlyingMode(BOOL fly);
	void setFlyingModeImpl(BOOL fly);
	static void setAlwaysRunMode(bool run);
	void setAlwaysRunModeImpl(bool run);
	static void setSittingMode(BOOL bSitting);
	static void enableInstance(BOOL bEnable);
	/*virtual*/ void onOpen(const LLSD& key);

	// *HACK: due to hard enough to have this control aligned with "Move" button while resizing
	// let update its position in each frame
	/*virtual*/ void draw(){updatePosition(); LLFloater::draw();}

	static void sUpdateFlyingStatus();

protected:
	void turnLeft();
	void turnRight();

	void moveUp();
	void moveDown();

private:
	typedef enum movement_mode_t
	{
		MM_WALK,
		MM_RUN,
		MM_FLY
	} EMovementMode;
	void onWalkButtonClick();
	void onRunButtonClick();
	void onFlyButtonClick();
	void onStopFlyingButtonClick();
	void initMovementMode();
	void setMovementMode(const EMovementMode mode);
	void showFlyControls(bool bShow);
	void initModeTooltips();
	void setModeTooltip(const EMovementMode mode);
	void showQuickTips(const EMovementMode mode);
	void initModeButtonMap();
	void setModeButtonToggleState(const EMovementMode mode);
	void updateButtonsWithMovementMode(const EMovementMode newMode);
	void updatePosition();
	void showModeButtons(BOOL bShow);

public:

	LLJoystickAgentTurn*	mForwardButton;
	LLJoystickAgentTurn*	mBackwardButton;
	LLButton*				mTurnLeftButton;
	LLButton*				mTurnRightButton;
	LLButton*				mMoveUpButton;
	LLButton*				mMoveDownButton;
private:
	LLButton*				mStopFlyingButton;
	LLPanel*				mModeActionsPanel;
	
	typedef std::map<LLView*, std::string> control_tooltip_map_t;
	typedef std::map<EMovementMode, control_tooltip_map_t> mode_control_tooltip_map_t;
	mode_control_tooltip_map_t mModeControlTooltipsMap;

	typedef std::map<EMovementMode, LLButton*> mode_control_button_map_t;
	mode_control_button_map_t mModeControlButtonMap;

};


/**
 * This class contains Stand Up and Stop Flying buttons displayed above Move button in bottom tray
 */
class LLPanelStandStopFlying : public LLPanel
{
public:
	typedef enum stand_stop_flying_mode_t
	{
		SSFM_STAND,
		SSFM_STOP_FLYING
	} EStandStopFlyingMode;

	static LLPanelStandStopFlying* getInstance();
	static void setStandStopFlyingMode(EStandStopFlyingMode mode);
	static void clearStandStopFlyingMode(EStandStopFlyingMode mode);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL visible);

	// *HACK: due to hard enough to have this control aligned with "Move" button while resizing
	// let update its position in each frame
	/*virtual*/ void draw(){updatePosition(); LLPanel::draw();}


protected:
	LLPanelStandStopFlying();


private:
	static LLPanelStandStopFlying* getStandStopFlyingPanel();
	void onStandButtonClick();
	void onStopFlyingButtonClick();
	void updatePosition();

	LLButton* mStandButton;
	LLButton* mStopFlyingButton;
};


#endif
