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
#include "llviewercontrol.h"
#include "llbutton.h"
#include "llviewerwindow.h"
#include "lljoystickbutton.h"
#include "lluictrlfactory.h"

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
:	LLFloater()
{
	setIsChrome(TRUE);

	const BOOL DONT_OPEN = FALSE;
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_moveview.xml", DONT_OPEN); 

}

// virtual
void LLFloaterMove::onClose(bool app_quitting)
{
	destroy();
	
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowMovementControls", FALSE);
	}
}
// virtual
BOOL LLFloaterMove::postBuild()
{
	mForwardButton = getChild<LLJoystickAgentTurn>("forward btn"); 
	mForwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mBackwardButton = getChild<LLJoystickAgentTurn>("backward btn"); 
	mBackwardButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mSlideLeftButton = getChild<LLJoystickAgentSlide>("slide left btn"); 
	mSlideLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mSlideRightButton = getChild<LLJoystickAgentSlide>("slide right btn"); 
	mSlideRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);

	mTurnLeftButton = getChild<LLButton>("turn left btn"); 
	mTurnLeftButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnLeftButton->setHeldDownCallback( turnLeft, NULL );

	mTurnRightButton = getChild<LLButton>("turn right btn"); 
	mTurnRightButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mTurnRightButton->setHeldDownCallback( turnRight, NULL );

	mMoveUpButton = getChild<LLButton>("move up btn"); 
	childSetAction("move up btn",moveUp,NULL);
	mMoveUpButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveUpButton->setHeldDownCallback( moveUp, NULL );

	mMoveDownButton = getChild<LLButton>("move down btn"); 
	childSetAction("move down btn",moveDown,NULL);	
	mMoveDownButton->setHeldDownDelay(MOVE_BUTTON_DELAY);
	mMoveDownButton->setHeldDownCallback( moveDown, NULL );
	return TRUE;
}
//
// Static member functions
//

void LLFloaterMove::onOpen(const LLSD& key)
{
	gSavedSettings.setBOOL("ShowMovementControls", TRUE);
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
	F32 time = getInstance()->mTurnLeftButton->getHeldDownTime();
	gAgent.moveYaw( getYawRate( time ) );
}

// protected static 
void LLFloaterMove::turnRight(void *)
{
	F32 time = getInstance()->mTurnRightButton->getHeldDownTime();
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
