/** 
 * @file llfloatercamera.cpp
 * @brief Container for camera control buttons (zoom, pan, orbit)
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
:	LLFloater(val)
{
	//// For now, only used for size and tooltip strings
	//const BOOL DONT_OPEN = FALSE;
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_camera.xml", DONT_OPEN);
}

// virtual
BOOL LLFloaterCamera::postBuild()
{
	setIsChrome(TRUE);

	mRotate = getChild<LLJoystickCameraRotate>("cam_rotate_stick");
	mZoom = getChild<LLJoystickCameraZoom>("zoom");
	mTrack = getChild<LLJoystickCameraTrack>("cam_track_stick");

	return TRUE;
}

// virtual
void LLFloaterCamera::onOpen(const LLSD& key)
{
	gSavedSettings.setBOOL("ShowCameraControls", TRUE);
}

// virtual
void LLFloaterCamera::onClose(bool app_quitting)
{
	destroy();
	
	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowCameraControls", FALSE);
	}
}

