/** 
 * @file llmoveview.cpp
 * @brief Container for movement buttons like forward, left, fly
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llcallbacklist.h"
#include "llviewercontrol.h"
#include "llfontgl.h"
#include "llbutton.h"
#include "llviewerwindow.h"
#include "lljoystickbutton.h"
#include "llresmgr.h"
#include "llvieweruictrlfactory.h"

//
// Constants
//

const F32 MOVE_BUTTON_DELAY = 0.0f;
const F32 YAW_NUDGE_RATE = 0.05f;	// fraction of normal speed
const F32 NUDGE_TIME = 0.25f;		// in seconds

const char *MOVE_TITLE = "";

//
// Global statics
//

LLFloaterMove* LLFloaterMove::sInstance = NULL;


//
// Member functions
//

// protected
LLFloaterMove::LLFloaterMove()
:	LLFloater("move floater", "FloaterMoveRect", MOVE_TITLE, FALSE, 100, 100, DRAG_ON_TOP,
			  MINIMIZE_NO)
{
	setIsChrome(TRUE);
	gUICtrlFactory->buildFloater(this,"floater_moveview.xml"); 

	mForwardButton = LLViewerUICtrlFactory::getJoystickAgentTurnByName(this, "forward btn"); 
	mForwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mBackwardButton = LLViewerUICtrlFactory::getJoystickAgentTurnByName(this, "backward btn"); 
	mBackwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mSlideLeftButton = LLViewerUICtrlFactory::getJoystickAgentSlideByName(this, "slide left btn"); 
	mSlideLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mSlideRightButton = LLViewerUICtrlFactory::getJoystickAgentSlideByName(this, "slide right btn"); 
	mSlideRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mTurnLeftButton = LLUICtrlFactory::getButtonByName(this, "turn left btn"); 
	mTurnLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnLeftButton->setHeldDownCallback( turnLeft );

	mTurnRightButton = LLUICtrlFactory::getButtonByName(this, "turn right btn"); 
	mTurnRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnRightButton->setHeldDownCallback( turnRight );

	mMoveUpButton = LLUICtrlFactory::getButtonByName(this, "move up btn"); 
	childSetAction("move up btn",moveUp,NULL);
	mMoveUpButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveUpButton->setHeldDownCallback( moveUp );

	mMoveDownButton = LLUICtrlFactory::getButtonByName(this, "move down btn"); 
	childSetAction("move down btn",moveDown,NULL);	
	mMoveDownButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveDownButton->setHeldDownCallback( moveDown );

	mFlyButton = LLUICtrlFactory::getButtonByName(this, "fly btn"); 
	childSetAction("fly btn",onFlyButtonClicked,NULL);

	sInstance = this;
}

// protected
LLFloaterMove::~LLFloaterMove()
{
	// children all deleted by LLView destructor
	sInstance = NULL;
}

// virtual
void LLFloaterMove::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
	
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowMovementControls", FALSE);
	}
}

//
// Static member functions
//

// static
void LLFloaterMove::show(void*)
{
	if (sInstance)
	{
		sInstance->open();	/*Flawfinder: ignore*/
	}
	else
	{
		LLFloaterMove* f = new LLFloaterMove();
		f->open();	/*Flawfinder: ignore*/
	}
	
	gSavedSettings.setBOOL("ShowMovementControls", TRUE);
}

// static
void LLFloaterMove::toggle(void*)
{
	if (sInstance)
	{
		sInstance->close();
	}
	else
	{
		show(NULL);
	}
}

// static
BOOL LLFloaterMove::visible(void*)
{
	return (sInstance != NULL);
}

// protected static 
void LLFloaterMove::onFlyButtonClicked(void *)
{
	gAgent.toggleFlying();
}


// protected static 
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

// protected static 
void LLFloaterMove::turnLeft(void *)
{
	F32 time = sInstance->mTurnLeftButton->getHeldDownTime();
	gAgent.moveYaw( getYawRate( time ) );
}

// protected static 
void LLFloaterMove::turnRight(void *)
{
	F32 time = sInstance->mTurnRightButton->getHeldDownTime();
	gAgent.moveYaw( -getYawRate( time ) );
}

// protected static 
void LLFloaterMove::moveUp(void *)
{
	// Jumps or flys up, depending on fly state
	gAgent.moveUp(1);
}

// protected static 
void LLFloaterMove::moveDown(void *)
{
	// Crouches or flys down, depending on fly state
	gAgent.moveUp(-1);
}

// EOF
