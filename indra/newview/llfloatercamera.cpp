/** 
 * @file llfloatercamera.cpp
 * @brief Container for camera control buttons (zoom, pan, orbit)
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

#include "llviewerprecompiledheaders.h"

#include "llfloatercamera.h"

// Library includes
#include "llfloaterreg.h"

// Viewer includes
#include "llagentcamera.h"
#include "lljoystickbutton.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llbottomtray.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"
#include "llslider.h"
#include "llfirstuse.h"
#include "llhints.h"

static LLDefaultChildRegistry::Register<LLPanelCameraItem> r("panel_camera_item");

const F32 NUDGE_TIME = 0.25f;		// in seconds
const F32 ORBIT_NUDGE_RATE = 0.05f; // fraction of normal speed

// Constants
const F32 CAMERA_BUTTON_DELAY = 0.0f;

#define ORBIT "cam_rotate_stick"
#define PAN "cam_track_stick"
#define ZOOM "zoom"
#define PRESETS "preset_views_list"
#define CONTROLS "controls"

bool LLFloaterCamera::sFreeCamera = false;
bool LLFloaterCamera::sAppearanceEditing = false;

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
	void	onCameraTrack();
	void	onCameraRotate();
	F32		getOrbitRate(F32 time);

private:
	LLButton*	mPlusBtn;
	LLButton*	mMinusBtn;
	LLSlider*	mSlider;
};

LLPanelCameraItem::Params::Params()
:	icon_over("icon_over"),
	icon_selected("icon_selected"),
	picture("picture"),
	text("text"),
	selected_picture("selected_picture"),
	mousedown_callback("mousedown_callback")
{
}

LLPanelCameraItem::LLPanelCameraItem(const LLPanelCameraItem::Params& p)
:	LLPanel(p)
{
	LLIconCtrl::Params icon_params = p.picture;
	mPicture = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mPicture);

	icon_params = p.icon_over;
	mIconOver = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mIconOver);

	icon_params = p.icon_selected;
	mIconSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mIconSelected);

	icon_params = p.selected_picture;
	mPictureSelected = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	addChild(mPictureSelected);

	LLTextBox::Params text_params = p.text;
	mText = LLUICtrlFactory::create<LLTextBox>(text_params);
	addChild(mText);

	if (p.mousedown_callback.isProvided())
	{
		setCommitCallback(initCommitCallback(p.mousedown_callback));
	}
}

void set_view_visible(LLView* parent, const std::string& name, bool visible)
{
	parent->getChildView(name)->setVisible(visible);
}

BOOL LLPanelCameraItem::postBuild()
{
	setMouseEnterCallback(boost::bind(set_view_visible, this, "hovered_icon", true));
	setMouseLeaveCallback(boost::bind(set_view_visible, this, "hovered_icon", false));
	setMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
	setRightMouseDownCallback(boost::bind(&LLPanelCameraItem::onAnyMouseClick, this));
	return TRUE;
}

void LLPanelCameraItem::onAnyMouseClick()
{
	if (mCommitSignal) (*mCommitSignal)(this, LLSD());
}

void LLPanelCameraItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	getChildView("selected_icon")->setVisible( value["selected"]);
	getChildView("picture")->setVisible( !value["selected"]);
	getChildView("selected_picture")->setVisible( value["selected"]);
}

static LLRegisterPanelClassWrapper<LLPanelCameraZoom> t_camera_zoom_panel("camera_zoom_panel");

//-------------------------------------------------------------------------------
// LLPanelCameraZoom
//-------------------------------------------------------------------------------

LLPanelCameraZoom::LLPanelCameraZoom()
:	mPlusBtn( NULL ),
	mMinusBtn( NULL ),
	mSlider( NULL )
{
	mCommitCallbackRegistrar.add("Zoom.minus", boost::bind(&LLPanelCameraZoom::onZoomMinusHeldDown, this));
	mCommitCallbackRegistrar.add("Zoom.plus", boost::bind(&LLPanelCameraZoom::onZoomPlusHeldDown, this));
	mCommitCallbackRegistrar.add("Slider.value_changed", boost::bind(&LLPanelCameraZoom::onSliderValueChanged, this));
	mCommitCallbackRegistrar.add("Camera.track", boost::bind(&LLPanelCameraZoom::onCameraTrack, this));
	mCommitCallbackRegistrar.add("Camera.rotate", boost::bind(&LLPanelCameraZoom::onCameraRotate, this));
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
	mSlider->setValue(gAgentCamera.getCameraZoomFraction());
	LLPanel::draw();
}

void LLPanelCameraZoom::onZoomPlusHeldDown()
{
	F32 val = mSlider->getValueF32();
	F32 inc = mSlider->getIncrement();
	mSlider->setValue(val - inc);
	F32 time = mPlusBtn->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitInKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onZoomMinusHeldDown()
{
	F32 val = mSlider->getValueF32();
	F32 inc = mSlider->getIncrement();
	mSlider->setValue(val + inc);
	F32 time = mMinusBtn->getHeldDownTime();
	gAgentCamera.unlockView();
	gAgentCamera.setOrbitOutKey(getOrbitRate(time));
}

void LLPanelCameraZoom::onCameraTrack()
{
	// EXP-202 when camera panning activated, remove the hint
	LLFirstUse::viewPopup( false );
}

void LLPanelCameraZoom::onCameraRotate()
{
	// EXP-202 when camera rotation activated, remove the hint
	LLFirstUse::viewPopup( false );
}

F32 LLPanelCameraZoom::getOrbitRate(F32 time)
{
	if( time < NUDGE_TIME )
	{
		F32 rate = ORBIT_NUDGE_RATE + time * (1 - ORBIT_NUDGE_RATE)/ NUDGE_TIME;
		return rate;
	}
	else
	{
		return 1;
	}
}

void  LLPanelCameraZoom::onSliderValueChanged()
{
	F32 zoom_level = mSlider->getValueF32();
	gAgentCamera.setCameraZoomFraction(zoom_level);
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
	if (floater_camera && floater_camera->mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA && gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK)
	{
		return true;
	}
	return false;
}

void LLFloaterCamera::resetCameraMode()
{
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (!floater_camera) return;
	floater_camera->switchMode(CAMERA_CTRL_MODE_PAN);
}

void LLFloaterCamera::onAvatarEditingAppearance(bool editing)
{
	sAppearanceEditing = editing;
	LLFloaterCamera* floater_camera = LLFloaterCamera::findInstance();
	if (!floater_camera) return;
	floater_camera->handleAvatarEditingAppearance(editing);
}

void LLFloaterCamera::handleAvatarEditingAppearance(bool editing)
{
	//camera presets (rear, front, etc.)
	getChildView("preset_views_list")->setEnabled(!editing);
	getChildView("presets_btn")->setEnabled(!editing);

	//camera modes (object view, mouselook view)
	getChildView("camera_modes_list")->setEnabled(!editing);
	getChildView("avatarview_btn")->setEnabled(!editing);
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
	if (floater_camera)
	{
		floater_camera->updateItemsSelection();
		if(floater_camera->inFreeCameraMode())
		{
			activate_camera_tool();
		}
	}
}

LLFloaterCamera* LLFloaterCamera::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterCamera>("camera");
}

void LLFloaterCamera::onOpen(const LLSD& key)
{
	LLFirstUse::viewPopup();

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
	// When mCurrMode is in CAMERA_CTRL_MODE_PAN
	// switchMode won't modify mPrevMode, so force it here.
	// It is needed to correctly return to previous mode on open, see EXT-2727.
	if (mCurrMode == CAMERA_CTRL_MODE_PAN)
		mPrevMode = CAMERA_CTRL_MODE_PAN;

	// HACK: Should always close as docked to prevent toggleInstance without calling onOpen.
	if ( !isDocked() )
		setDocked(true);
	switchMode(CAMERA_CTRL_MODE_PAN);
	mClosed = TRUE;
}

LLFloaterCamera::LLFloaterCamera(const LLSD& val)
:	LLTransientDockableFloater(NULL, true, val),
	mClosed(FALSE),
	mCurrMode(CAMERA_CTRL_MODE_PAN),
	mPrevMode(CAMERA_CTRL_MODE_PAN)
{
	LLHints::registerHintTarget("view_popup", LLView::getHandle());
}

// virtual
BOOL LLFloaterCamera::postBuild()
{
	setIsChrome(TRUE);
	setTitleVisible(TRUE); // restore title visibility after chrome applying

	mRotate = getChild<LLJoystickCameraRotate>(ORBIT);
	mZoom = findChild<LLPanelCameraZoom>(ZOOM);
	mTrack = getChild<LLJoystickCameraTrack>(PAN);

	assignButton2Mode(CAMERA_CTRL_MODE_MODES,			"avatarview_btn");
	assignButton2Mode(CAMERA_CTRL_MODE_PAN,				"pan_btn");
	assignButton2Mode(CAMERA_CTRL_MODE_PRESETS,		"presets_btn");

	update();

	// ensure that appearance mode is handled while building. See EXT-7796.
	handleAvatarEditingAppearance(sAppearanceEditing);

	return LLDockableFloater::postBuild();
}

void LLFloaterCamera::fillFlatlistFromPanel (LLFlatListView* list, LLPanel* panel)
{
	// copying child list and then iterating over a copy, because list itself
	// is changed in process
	const child_list_t child_list = *panel->getChildList();
	child_list_t::const_reverse_iterator iter = child_list.rbegin();
	child_list_t::const_reverse_iterator end = child_list.rend();
	for ( ; iter != end; ++iter)
	{
		LLView* view = *iter;
		LLPanel* item = dynamic_cast<LLPanel*>(view);
		if (panel)
			list->addItem(item);
	}

}

ECameraControlMode LLFloaterCamera::determineMode()
{
	if (sAppearanceEditing)
	{
		// this is the only enabled camera mode while editing agent appearance.
		return CAMERA_CTRL_MODE_PAN;
	}

	LLTool* curr_tool = LLToolMgr::getInstance()->getCurrentTool();
	if (curr_tool == LLToolCamera::getInstance())
	{
		return CAMERA_CTRL_MODE_FREE_CAMERA;
	} 

	if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK)
	{
		return CAMERA_CTRL_MODE_PRESETS;
	}

	return CAMERA_CTRL_MODE_PAN;
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

void LLFloaterCamera::setModeTitle(const ECameraControlMode mode)
{
	std::string title; 
	switch(mode)
	{
	case CAMERA_CTRL_MODE_MODES:
		title = getString("camera_modes_title");
		break;
	case CAMERA_CTRL_MODE_PAN:
		title = getString("pan_mode_title");
		break;
	case CAMERA_CTRL_MODE_PRESETS:
		title = getString("presets_mode_title");
		break;
	default:
		break;
	}
	setTitle(title);
}

void LLFloaterCamera::switchMode(ECameraControlMode mode)
{
	setMode(mode);

	switch (mode)
	{
	case CAMERA_CTRL_MODE_MODES:
		if(sFreeCamera)
		{
			switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
		}
		break;

	case CAMERA_CTRL_MODE_PAN:
		sFreeCamera = false;
		clear_camera_tool();
		break;

	case CAMERA_CTRL_MODE_FREE_CAMERA:
		sFreeCamera = true;
		activate_camera_tool();
		break;

	case CAMERA_CTRL_MODE_PRESETS:
		if(sFreeCamera)
		{
			switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
		}
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
	getChildView(ZOOM)->setVisible(CAMERA_CTRL_MODE_PAN == mCurrMode);
	
	bool show_presets = (CAMERA_CTRL_MODE_PRESETS == mCurrMode) || (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode
																	&& CAMERA_CTRL_MODE_PRESETS == mPrevMode);
	getChildView(PRESETS)->setVisible(show_presets);
	
	bool show_camera_modes = CAMERA_CTRL_MODE_MODES == mCurrMode || (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode
																	&& CAMERA_CTRL_MODE_MODES == mPrevMode);
	getChildView("camera_modes_list")->setVisible( show_camera_modes);

	updateItemsSelection();

	if (CAMERA_CTRL_MODE_FREE_CAMERA == mCurrMode)
	{
		return;
	}

	//updating buttons
	std::map<ECameraControlMode, LLButton*>::const_iterator iter = mMode2Button.begin();
	for (; iter != mMode2Button.end(); ++iter)
	{
		iter->second->setToggleState(iter->first == mCurrMode);
	}
	setModeTitle(mCurrMode);
}

void LLFloaterCamera::updateItemsSelection()
{
	ECameraPreset preset = (ECameraPreset) gSavedSettings.getU32("CameraPreset");
	LLSD argument;
	argument["selected"] = preset == CAMERA_PRESET_REAR_VIEW;
	getChild<LLPanelCameraItem>("rear_view")->setValue(argument);
	argument["selected"] = preset == CAMERA_PRESET_GROUP_VIEW;
	getChild<LLPanelCameraItem>("group_view")->setValue(argument);
	argument["selected"] = preset == CAMERA_PRESET_FRONT_VIEW;
	getChild<LLPanelCameraItem>("front_view")->setValue(argument);
	argument["selected"] = gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK;
	getChild<LLPanelCameraItem>("mouselook_view")->setValue(argument);
	argument["selected"] = mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA;
	getChild<LLPanelCameraItem>("object_view")->setValue(argument);
}

void LLFloaterCamera::onClickCameraItem(const LLSD& param)
{
	std::string name = param.asString();

	if ("mouselook_view" == name)
	{
		gAgentCamera.changeCameraToMouselook();
	}
	else if ("object_view" == name)
	{
		LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
		if (camera_floater)
		camera_floater->switchMode(CAMERA_CTRL_MODE_FREE_CAMERA);
	}
	else
	{
		switchToPreset(name);
	}

	LLFloaterCamera* camera_floater = LLFloaterCamera::findInstance();
	if (camera_floater)
	{
		camera_floater->updateItemsSelection();
		camera_floater->fromFreeToPresets();
	}
}

/*static*/
void LLFloaterCamera::switchToPreset(const std::string& name)
{
	sFreeCamera = false;
	clear_camera_tool();
	if ("rear_view" == name)
	{
		gAgentCamera.switchCameraPreset(CAMERA_PRESET_REAR_VIEW);
	}
	else if ("group_view" == name)
	{
		gAgentCamera.switchCameraPreset(CAMERA_PRESET_GROUP_VIEW);
	}
	else if ("front_view" == name)
	{
		gAgentCamera.switchCameraPreset(CAMERA_PRESET_FRONT_VIEW);
	}
}

void LLFloaterCamera::fromFreeToPresets()
{
	if (!sFreeCamera && mCurrMode == CAMERA_CTRL_MODE_FREE_CAMERA && mPrevMode == CAMERA_CTRL_MODE_PRESETS)
	{
		switchMode(CAMERA_CTRL_MODE_PRESETS);
	}
}
