/** 
 * @file llmoveview.h
 * @brief Container for buttons for walking, turning, flying
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
	LOG_CLASS(LLFloaterMove);
	friend class LLFloaterReg;

private:
	LLFloaterMove(const LLSD& key);
	~LLFloaterMove();
public:

	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void	setVisible(BOOL visible);
	static F32	getYawRate(F32 time);
	static void setFlyingMode(BOOL fly);
	void setFlyingModeImpl(BOOL fly);
	static void setAlwaysRunMode(bool run);
	void setAlwaysRunModeImpl(bool run);
	static void setSittingMode(BOOL bSitting);
	static void enableInstance(BOOL bEnable);
	/*virtual*/ void onOpen(const LLSD& key);

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
	void initMovementMode();
	void setMovementMode(const EMovementMode mode);
	void initModeTooltips();
	void setModeTooltip(const EMovementMode mode);
	void setModeTitle(const EMovementMode mode);
	void initModeButtonMap();
	void setModeButtonToggleState(const EMovementMode mode);
	void updateButtonsWithMovementMode(const EMovementMode newMode);
	void showModeButtons(BOOL bShow);

public:

	LLJoystickAgentTurn*	mForwardButton;
	LLJoystickAgentTurn*	mBackwardButton;
	LLJoystickAgentSlide*	mSlideLeftButton;
	LLJoystickAgentSlide*	mSlideRightButton;
	LLButton*				mTurnLeftButton;
	LLButton*				mTurnRightButton;
	LLButton*				mMoveUpButton;
	LLButton*				mMoveDownButton;
private:
	LLPanel*				mModeActionsPanel;
	
	typedef std::map<LLView*, std::string> control_tooltip_map_t;
	typedef std::map<EMovementMode, control_tooltip_map_t> mode_control_tooltip_map_t;
	mode_control_tooltip_map_t mModeControlTooltipsMap;

	typedef std::map<EMovementMode, LLButton*> mode_control_button_map_t;
	mode_control_button_map_t mModeControlButtonMap;
	EMovementMode mCurrentMode;

};


/**
 * This class contains Stand Up and Stop Flying buttons displayed above Move button in bottom tray
 */
class LLPanelStandStopFlying : public LLPanel
{
	LOG_CLASS(LLPanelStandStopFlying);
public:
	typedef enum stand_stop_flying_mode_t
	{
		SSFM_STAND,
		SSFM_STOP_FLYING
	} EStandStopFlyingMode;

	/**
	 * Attach or detach the panel to/from the movement controls floater.
	 * 
	 * Called when the floater gets opened/closed, user sits, stands up or starts/stops flying.
	 * 
	 * @param move_view The floater to attach to (not always accessible via floater registry).
	 *        If NULL is passed, the panel gets reparented to its original container.
	 *
	 * @see mAttached
	 * @see mOriginalParent 
	 */
	void reparent(LLFloaterMove* move_view);

	static LLPanelStandStopFlying* getInstance();
	static void setStandStopFlyingMode(EStandStopFlyingMode mode);
	static void clearStandStopFlyingMode(EStandStopFlyingMode mode);
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL visible);

	// *HACK: due to hard enough to have this control aligned with "Move" button while resizing
	// let update its position in each frame
	/*virtual*/ void draw(){updatePosition(); LLPanel::draw();}
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);


protected:
	LLPanelStandStopFlying();


private:
	static LLPanelStandStopFlying* getStandStopFlyingPanel();
	void onStandButtonClick();
	void onStopFlyingButtonClick();
	void updatePosition();

	LLButton* mStandButton;
	LLButton* mStopFlyingButton;

	/**
	 * The original parent of the panel.
	 *  
	 * Makes it possible to move (reparent) the panel to the movement controls floater and back.
	 * 
	 * @see reparent()
	 */
	LLHandle<LLPanel> mOriginalParent;

	/**
	 * True if the panel is currently attached to the movement controls floater.
	 * 
	 * @see reparent()
	 * @see updatePosition()
	 */
	bool	mAttached;
};


#endif
