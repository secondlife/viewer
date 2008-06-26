/** 
 * @file llfloatercamera.cpp
 * @brief Container for camera control buttons (zoom, pan, orbit)
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

#include "llfloatercamera.h"

// Library includes
#include "lluictrlfactory.h"

// Viewer includes
#include "lljoystickbutton.h"
#include "llviewercontrol.h"

// Constants
const F32 CAMERA_BUTTON_DELAY = 0.0f;

//
// Member functions
//

LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:	LLFloater("camera floater") // uses "FloaterCameraRect3"
{
	setIsChrome(TRUE);
	
	// For now, only used for size and tooltip strings
	const BOOL DONT_OPEN = FALSE;
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_camera.xml", NULL, DONT_OPEN);
	
	S32 top = getRect().getHeight();
	S32 bottom = 0;
	S32 left = 16;
	
	const S32 ROTATE_WIDTH = 64;
	mRotate = new LLJoystickCameraRotate(std::string("cam rotate stick"), 
										 LLRect( left, top, left + ROTATE_WIDTH, bottom ),
										 std::string("cam_rotate_out.tga"),
										 std::string("cam_rotate_in.tga") );
	mRotate->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
	mRotate->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mRotate->setToolTip( getString("rotate_tooltip") );
	mRotate->setSoundFlags(MOUSE_DOWN | MOUSE_UP);
	addChild(mRotate);
	
	left += ROTATE_WIDTH;
	
	const S32 ZOOM_WIDTH = 16;
	mZoom = new LLJoystickCameraZoom( 
									 std::string("zoom"),
									 LLRect( left, top, left + ZOOM_WIDTH, bottom ),
									 std::string("cam_zoom_out.tga"),
									 std::string("cam_zoom_plus_in.tga"),
									 std::string("cam_zoom_minus_in.tga"));
	mZoom->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
	mZoom->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mZoom->setToolTip( getString("zoom_tooltip") );
	mZoom->setSoundFlags(MOUSE_DOWN | MOUSE_UP);
	addChild(mZoom);
	
	left += ZOOM_WIDTH;
	
	const S32 TRACK_WIDTH = 64;
	mTrack = new LLJoystickCameraTrack(std::string("cam track stick"), 
									   LLRect( left, top, left + TRACK_WIDTH, bottom ),
									   std::string("cam_tracking_out.tga"),
									   std::string("cam_tracking_in.tga"));
	mTrack->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT);
	mTrack->setHeldDownDelay(CAMERA_BUTTON_DELAY);
	mTrack->setToolTip( getString("move_tooltip") );
	mTrack->setSoundFlags(MOUSE_DOWN | MOUSE_UP);
	addChild(mTrack);
}

// virtual
void LLFloaterCamera::onOpen()
{
	LLFloater::onOpen();
	
	gSavedSettings.setBOOL("ShowCameraControls", TRUE);
}

// virtual
void LLFloaterCamera::onClose(bool app_quitting)
{
	LLFloater::onClose(app_quitting);
	
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowCameraControls", FALSE);
	}
}
