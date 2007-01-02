/** 
 * @file llmoveview.h
 * @brief Container for buttons for walking, turning, flying
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
protected:
	LLFloaterMove();
	~LLFloaterMove();

public:
	/*virtual*/ void onClose(bool app_quitting);
	static void onFlyButtonClicked(void* userdata);
	static F32	getYawRate(F32 time);

	static void show(void*);
	static void toggle(void*);
	static BOOL visible(void*);
	
	// HACK for agent-driven button highlighting
	static LLFloaterMove* getInstance()				{ return sInstance; }

protected:
	static void turnLeftNudge(void* userdata);
	static void turnLeft(void* userdata);
	
	static void turnRightNudge(void* userdata);
	static void turnRight(void* userdata);

	static void moveUp(void* userdata);
	static void moveDown(void* userdata);

public:
	LLButton*				mFlyButton;

	LLJoystickAgentTurn*	mForwardButton;
	LLJoystickAgentTurn*	mBackwardButton;
	LLJoystickAgentSlide*	mSlideLeftButton;
	LLJoystickAgentSlide*	mSlideRightButton;
	LLButton*				mTurnLeftButton;
	LLButton*				mTurnRightButton;
	LLButton*				mMoveUpButton;
	LLButton*				mMoveDownButton;

protected:
	static LLFloaterMove*	sInstance;
};


#endif
