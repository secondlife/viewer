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
#include "llfloaterreg.h"

// Viewer includes
#include "lljoystickbutton.h"
#include "llfirsttimetipmanager.h"
#include "llviewercontrol.h"
#include "llbottomtray.h"
#include "llagent.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"

// Constants
const F32 CAMERA_BUTTON_DELAY = 0.0f;

#define ORBIT "cam_rotate_stick"
#define PAN "cam_track_stick"
#define CONTROLS "controls"


void show_tip(LLFirstTimeTipsManager::EFirstTimeTipType tipType, LLView* anchorView)
{
	LLFirstTimeTipsManager::showTipsFor(tipType, anchorView, LLFirstTimeTipsManager::TPA_POS_RIGHT_ALIGN_TOP);
}
//
// Member functions
//

/*static*/ bool LLFloaterCamera::inFreeCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera && floater_camera->mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA)
	{
		return true;
	}
	return false;
}

bool LLFloaterCamera::inAvatarViewMode()
{
	return mCurrMode == CAMERA_CTRL_MODE_AVATAR_VIEW;
}

void LLFloaterCamera::resetCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (!floater_camera) return;
	floater_camera->switchMode(CAMERA_CTRL_MODE_ORBIT);
}

void LLFloaterCamera::update()
{
	ECameraControlMode mode = determineMode();
	if (mode != mCurrMode) setMode(mode);
	show_tip(mMode2TipType[mode], this);
}


/*static*/ void LLFloaterCamera::updateIfNotInAvatarViewMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera && !floater_camera->inAvatarViewMode()) 
	{
		floater_camera->update();
	}
}


void LLFloaterCamera::toPrevMode()
{
	switchMode(mPrevMode);
}

/*static*/ void LLFloaterCamera::toPrevModeIfInAvatarViewMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera && floater_camera->inAvatarViewMode())
	{
		floater_camera->toPrevMode();
	}
}

LLFloaterCamera* LLFloaterCamera::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterCamera>("camera");
}

void LLFloaterCamera::onOpen(const LLSD& key)
{
	LLButton *anchor_panel = LLBottomTray::getInstance()->getChild<LLButton>("camera_btn");

	setDockControl(new LLDockControl(
		anchor_panel, this,
		getDockTongue(), LLDockControl::TOP));

	show_tip(mMode2TipType[mCurrMode], this);
}


LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:	LLDockableFloater(NULL, false, val),
	mCurrMode(CAMERA_CTRL_MODE_ORBIT),
	mPrevMode(CAMERA_CTRL_MODE_ORBIT)
{
}

// virtual
BOOL LLFloaterCamera::postBuild()
{
	setIsChrome(TRUE);

	mRotate = getChild<LLJoystickCameraRotate>(ORBIT);
	mZoom = getChild<LLJoystickCameraZoom>("zoom");
	mTrack = getChild<LLJoystickCameraTrack>(PAN);

	initMode2TipTypeMap();

	assignButton2Mode(CAMERA_CTRL_MODE_ORBIT,			"orbit_btn");
	assignButton2Mode(CAMERA_CTRL_MODE_PAN,				"pan_btn");
	assignButton2Mode(CAMERA_CTRL_MODE_FREE_CAMERA,		"freecamera_btn");
	assignButton2Mode(CAMERA_CTRL_MODE_AVATAR_VIEW,		"avatarview_btn");

	update();

	return LLDockableFloater::postBuild();
}

ECameraControlMode LLFloaterCamera::determineMode()
{
	LLTool* curr_tool = LLToolMgr::getInstance()->getCurrentTool();
	if (curr_tool == LLToolCamera::getInstance())
	{
		return CAMERA_CTRL_MODE_FREE_CAMERA;
	} 

	if (gAgent.getCameraMode() == CAMERA_MODE_MOUSELOOK)
	{
		return CAMERA_CTRL_MODE_AVATAR_VIEW;
	}

	return CAMERA_CTRL_MODE_ORBIT;
}


void clear_camera_tool()
{
	LLToolMgr* tool_mgr = LLToolMgr::getInstance();
	if (tool_mgr->usingTransientTool() && 
		tool_mgr->getCurrentTool() == LLToolCamera::getInstance())
	{
		tool_mgr->clearTransientTool();
	}
}


void LLFloaterCamera::setMode(ECameraControlMode mode)
{
	if (mode != mCurrMode)
	{
		mPrevMode = mCurrMode;
		mCurrMode = mode;
	}
	
	updateState();
}

void LLFloaterCamera::switchMode(ECameraControlMode mode)
{
	setMode(mode);

	switch (mode)
	{
	case CAMERA_CTRL_MODE_ORBIT:
		clear_camera_tool();
		break;

	case CAMERA_CTRL_MODE_PAN:
		clear_camera_tool();
		break;

	case CAMERA_CTRL_MODE_FREE_CAMERA:
		LLToolMgr::getInstance()->setTransientTool(LLToolCamera::getInstance());
		break;

	case CAMERA_CTRL_MODE_AVATAR_VIEW:
		gAgent.changeCameraToMouselook();
		break;

	default:
		//normally we won't occur here
		llassert_always(FALSE);
	}
}


void LLFloaterCamera::onClickBtn(ECameraControlMode mode)
{
	// check for a click on active button
	if (mCurrMode == mode) mMode2Button[mode]->setToggleState(TRUE);
	
	switchMode(mode);

	show_tip(mMode2TipType[mode], this);
}

void LLFloaterCamera::assignButton2Mode(ECameraControlMode mode, const std::string& button_name)
{
	LLButton* button = getChild<LLButton>(button_name);
	
	button->setClickedCallback(boost::bind(&LLFloaterCamera::onClickBtn, this, mode));
	mMode2Button[mode] = button;
}

void LLFloaterCamera::initMode2TipTypeMap()
{
	mMode2TipType[CAMERA_CTRL_MODE_ORBIT]			= LLFirstTimeTipsManager::FTT_CAMERA_ORBIT_MODE;
	mMode2TipType[CAMERA_CTRL_MODE_PAN]				= LLFirstTimeTipsManager::FTT_CAMERA_PAN_MODE;
	mMode2TipType[CAMERA_CTRL_MODE_FREE_CAMERA]		= LLFirstTimeTipsManager::FTT_CAMERA_FREE_MODE;
	mMode2TipType[CAMERA_CTRL_MODE_AVATAR_VIEW]		= LLFirstTimeTipsManager::FTT_AVATAR_FREE_MODE;
}


void LLFloaterCamera::updateState()
{
	//updating buttons
	std::map<ECameraControlMode, LLButton*>::const_iterator iter = mMode2Button.begin();
	for (; iter != mMode2Button.end(); ++iter)
	{
		iter->second->setToggleState(iter->first == mCurrMode);
	}

	//updating controls
	bool isOrbitMode = CAMERA_CTRL_MODE_ORBIT == mCurrMode;
	bool isPanMode = CAMERA_CTRL_MODE_PAN == mCurrMode;

	childSetVisible(ORBIT, isOrbitMode);
	childSetVisible(PAN, isPanMode);

	//hiding or showing the panel with controls by reshaping the floater
	bool showControls = isOrbitMode || isPanMode;
	if (showControls == childIsVisible(CONTROLS)) return;

	childSetVisible(CONTROLS, showControls);

	LLRect rect = getRect();
	LLRect controls_rect;
	if (childGetRect(CONTROLS, controls_rect))
	{
		static LLUICachedControl<S32> floater_header_size ("UIFloaterHeaderSize", 0);
		static S32 height = controls_rect.getHeight() - floater_header_size;
		S32 newHeight = rect.getHeight();
		
		if (showControls)
		{
			newHeight += height;
		}
		else
		{
			newHeight -= height;
		}

		rect.setOriginAndSize(rect.mLeft, rect.mBottom, rect.getWidth(), newHeight);
		reshape(rect.getWidth(), rect.getHeight());
		setRect(rect);

	}
}

//-------------LLFloaterCameraPresets------------------------

LLFloaterCameraPresets::LLFloaterCameraPresets(const LLSD& key):
LLDockableFloater(NULL, false, key)
{}

BOOL LLFloaterCameraPresets::postBuild()
{
	setIsChrome(TRUE);

	//build dockTongue 
	LLDockableFloater::postBuild();
	 
	LLButton *anchor_btn = LLBottomTray::getInstance()->getChild<LLButton>("camera_presets_btn");

		setDockControl(new LLDockControl(
			anchor_btn, this,
			getDockTongue(), LLDockControl::TOP));
		return TRUE;
}

/*static*/
void LLFloaterCameraPresets::onClickCameraPresets(LLUICtrl* ctrl, const LLSD& param)
{
	std::string name = param.asString();

	if ("rear_view" == name)
	{
		LLFirstTimeTipsManager::showTipsFor(LLFirstTimeTipsManager::FTT_CAMERA_PRESET_REAR, ctrl);
		gAgent.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
	}
	else if ("group_view" == name)
	{
		LLFirstTimeTipsManager::showTipsFor(LLFirstTimeTipsManager::FTT_CAMERA_PRESET_GROUP);
		gAgent.switchCameraPreset(CAMERA_PRESET_GROUP_VIEW);
	}
	else if ("front_view" == name)
	{
		LLFirstTimeTipsManager::showTipsFor(LLFirstTimeTipsManager::FTT_CAMERA_PRESET_FRONT);
		gAgent.switchCameraPreset(CAMERA_PRESET_FRONT_VIEW);
	}

}
