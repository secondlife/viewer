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
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llbottomtray.h"
#include "llagent.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"
#include "llslider.h"

// Constants
const F32 CAMERA_BUTTON_DELAY = 0.0f;

#define ORBIT "cam_rotate_stick"
#define PAN "cam_track_stick"
#define ZOOM "zoom"
#define PRESETS "camera_presets"
#define CONTROLS "controls"

// Zoom the camera in and out
class LLPanelCameraZoom
:	public LLPanel
{
	LOG_CLASS(LLPanelCameraZoom);
public:
	LLPanelCameraZoom();

	/* virtual */ BOOL	postBuild();
	/* virtual */ void	draw();

protected:
	void	onZoomPlusHeldDown();
	void	onZoomMinusHeldDown();
	void	onSliderValueChanged();

private:
	LLButton*	mPlusBtn;
	LLButton*	mMinusBtn;
	LLSlider*	mSlider;
};

static LLRegisterPanelClassWrapper<LLPanelCameraZoom> t_camera_zoom_panel("camera_zoom_panel");

//-------------------------------------------------------------------------------
// LLPanelCameraZoom
//-------------------------------------------------------------------------------

LLPanelCameraZoom::LLPanelCameraZoom()
:	mPlusBtn( NULL ),
	mMinusBtn( NULL ),
	mSlider( NULL )
{
	mCommitCallbackRegistrar.add("Zoom.minus", boost::bind(&LLPanelCameraZoom::onZoomPlusHeldDown, this));
	mCommitCallbackRegistrar.add("Zoom.plus", boost::bind(&LLPanelCameraZoom::onZoomMinusHeldDown, this));
	mCommitCallbackRegistrar.add("Slider.value_changed", boost::bind(&LLPanelCameraZoom::onSliderValueChanged, this));
}

BOOL LLPanelCameraZoom::postBuild()
{
	mPlusBtn  = getChild <LLButton> ("zoom_plus_btn");
	mMinusBtn = getChild <LLButton> ("zoom_minus_btn");
	mSlider   = getChild <LLSlider> ("zoom_slider");
	return LLPanel::postBuild();
}

void LLPanelCameraZoom::draw()
{
	mSlider->setValue(gAgent.getCameraZoomFraction());
	LLPanel::draw();
}

void LLPanelCameraZoom::onZoomPlusHeldDown()
{
	F32 val = mSlider->getValueF32();
	F32 inc = mSlider->getIncrement();
	mSlider->setValue(val - inc);
	// commit only if value changed
	if (val != mSlider->getValueF32())
		mSlider->onCommit();
}

void LLPanelCameraZoom::onZoomMinusHeldDown()
{
	F32 val = mSlider->getValueF32();
	F32 inc = mSlider->getIncrement();
	mSlider->setValue(val + inc);
	// commit only if value changed
	if (val != mSlider->getValueF32())
		mSlider->onCommit();
}

void  LLPanelCameraZoom::onSliderValueChanged()
{
	F32 zoom_level = mSlider->getValueF32();
	gAgent.setCameraZoomFraction(zoom_level);
}

void activate_camera_tool()
{
	LLToolMgr::getInstance()->setTransientTool(LLToolCamera::getInstance());
};

//
// Member functions
//

/*static*/ bool LLFloaterCamera::inFreeCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera && floater_camera->mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA && gAgent.getCameraMode() != CAMERA_MODE_MOUSELOOK)
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
}


void LLFloaterCamera::toPrevMode()
{
	switchMode(mPrevMode);
}

/*static*/ void LLFloaterCamera::onLeavingMouseLook()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (floater_camera && floater_camera->inFreeCameraMode())
	{
		activate_camera_tool();
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

	mZoom->onOpen(key);

	// Returns to previous mode, see EXT-2727(View tool should remember state).
	// In case floater was just hidden and it isn't reset the mode
	// just update state to current one. Else go to previous.
	if ( !mClosed )
		updateState();
	else
		toPrevMode();
	mClosed = FALSE;
}

void LLFloaterCamera::onClose(bool app_quitting)
{
	//We don't care of camera mode if app is quitting
	if(app_quitting)
		return;
	// When mCurrMode is in CAMERA_CTRL_MODE_ORBIT
	// switchMode won't modify mPrevMode, so force it here.
	// It is needed to correctly return to previous mode on open, see EXT-2727.
	if (mCurrMode == CAMERA_CTRL_MODE_ORBIT)
		mPrevMode = CAMERA_CTRL_MODE_ORBIT;

	// HACK: Should always close as docked to prevent toggleInstance without calling onOpen.
	if ( !isDocked() )
		setDocked(true);
	switchMode(CAMERA_CTRL_MODE_ORBIT);
	mClosed = TRUE;
}

LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:	LLTransientDockableFloater(NULL, true, val),
	mClosed(FALSE),
	mCurrMode(CAMERA_CTRL_MODE_ORBIT),
	mPrevMode(CAMERA_CTRL_MODE_ORBIT)
{
}

// virtual
BOOL LLFloaterCamera::postBuild()
{
	setIsChrome(TRUE);

	mRotate = getChild<LLJoystickCameraRotate>(ORBIT);
	mZoom = getChild<LLPanelCameraZoom>(ZOOM);
	mTrack = getChild<LLJoystickCameraTrack>(PAN);

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
		activate_camera_tool();
		break;

	case CAMERA_CTRL_MODE_AVATAR_VIEW:
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

}

void LLFloaterCamera::assignButton2Mode(ECameraControlMode mode, const std::string& button_name)
{
	LLButton* button = getChild<LLButton>(button_name);
	
	button->setClickedCallback(boost::bind(&LLFloaterCamera::onClickBtn, this, mode));
	mMode2Button[mode] = button;
}

void LLFloaterCamera::updateState()
{
	//updating buttons
	std::map<ECameraControlMode, LLButton*>::const_iterator iter = mMode2Button.begin();
	for (; iter != mMode2Button.end(); ++iter)
	{
		iter->second->setToggleState(iter->first == mCurrMode);
	}

	childSetVisible(ORBIT, CAMERA_CTRL_MODE_ORBIT == mCurrMode);
	childSetVisible(PAN, CAMERA_CTRL_MODE_PAN == mCurrMode);
	childSetVisible(ZOOM, CAMERA_CTRL_MODE_AVATAR_VIEW != mCurrMode);
	childSetVisible(PRESETS, CAMERA_CTRL_MODE_AVATAR_VIEW == mCurrMode);

	//hiding or showing the panel with controls by reshaping the floater
	bool showControls = CAMERA_CTRL_MODE_FREE_CAMERA != mCurrMode;
	if (showControls == childIsVisible(CONTROLS)) return;

	childSetVisible(CONTROLS, showControls);

	LLRect rect = getRect();
	LLRect controls_rect;
	if (childGetRect(CONTROLS, controls_rect))
	{
		S32 floater_header_size = getHeaderHeight();
		S32 height = controls_rect.getHeight() - floater_header_size;
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

void LLFloaterCamera::onClickCameraPresets(const LLSD& param)
{
	std::string name = param.asString();

	if ("rear_view" == name)
	{
		gAgent.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
	}
	else if ("group_view" == name)
	{
		gAgent.switchCameraPreset(CAMERA_PRESET_GROUP_VIEW);
	}
	else if ("front_view" == name)
	{
		gAgent.switchCameraPreset(CAMERA_PRESET_FRONT_VIEW);
	}
	else if ("mouselook_view" == name)
	{
		gAgent.changeCameraToMouselook();
	}

}
