/** 
 * @file llfloatercamera.h
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

#ifndef LLFLOATERCAMERA_H
#define LLFLOATERCAMERA_H

#include "llfloater.h"
#include "lliconctrl.h"
#include "lltextbox.h"
#include "llflatlistview.h"

class LLJoystickCameraRotate;
class LLJoystickCameraTrack;
class LLFloaterReg;
class LLPanelCameraZoom;
class LLComboBox;

enum ECameraControlMode
{
	CAMERA_CTRL_MODE_PAN,
	CAMERA_CTRL_MODE_FREE_CAMERA,
	CAMERA_CTRL_MODE_PRESETS
};

class LLFloaterCamera : public LLFloater
{
	friend class LLFloaterReg;
	
public:
	/* whether in free camera mode */
	static bool inFreeCameraMode();
	/* callback for camera items selection changing */
	static void onClickCameraItem(const LLSD& param);

	static void onLeavingMouseLook();

	/** resets current camera mode to orbit mode */
	static void resetCameraMode();

	/** Called when Avatar is entered/exited editing appearance mode */
	static void onAvatarEditingAppearance(bool editing);

	/* determines actual mode and updates ui */
	void update();

	/*switch to one of the camera presets (front, rear, side)*/
	static void switchToPreset(const std::string& name);

	virtual void onOpen(const LLSD& key);
	virtual void onClose(bool app_quitting);

	void onSavePreset();
	void onCustomPresetSelected();

	void populatePresetCombo();

	LLJoystickCameraRotate* mRotate;
	LLPanelCameraZoom*	mZoom;
	LLJoystickCameraTrack*	mTrack;

private:

	LLFloaterCamera(const LLSD& val);
	~LLFloaterCamera() {};

	/* return instance if it exists - created by LLFloaterReg */
	static LLFloaterCamera* findInstance();

	/*virtual*/ bool postBuild();

	F32 getCurrentTransparency();

	void onViewButtonClick(const LLSD& user_data);

	ECameraControlMode determineMode();

	/* resets to the previous mode */
	void toPrevMode();

	/* sets a new mode and performs related actions */
	void switchMode(ECameraControlMode mode);

	/* sets a new mode preserving previous one and updates ui*/
	void setMode(ECameraControlMode mode);

	/* updates the state (UI) according to the current mode */
	void updateState();

	/* update camera modes items selection and camera preset items selection according to the currently selected preset */
	void updateItemsSelection();

	// fills flatlist with items from given panel
	void fillFlatlistFromPanel (LLFlatListView* list, LLPanel* panel);

	void handleAvatarEditingAppearance(bool editing);

	// set to true when free camera mode is selected in modes list
	// remains true until preset camera mode is chosen, or pan button is clicked, or escape pressed
	static bool sFreeCamera;
	static bool sAppearanceEditing;
	bool mClosed;
	ECameraControlMode mPrevMode;
	ECameraControlMode mCurrMode;
	std::map<ECameraControlMode, LLButton*> mMode2Button;

	LLComboBox* mPresetCombo;
};

/**
 * Class used to represent widgets from panel_camera_item.xml- 
 * panels that contain pictures and text. Pictures are different
 * for selected and unselected state (this state is nor stored- icons
 * are changed in setValue()). This class doesn't implement selection logic-
 * it's items are used inside of flatlist.
 */
class LLPanelCameraItem 
	: public LLPanel
{
public:
	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<LLIconCtrl::Params> icon_over;
		Optional<LLIconCtrl::Params> icon_selected;
		Optional<LLIconCtrl::Params> picture;
		Optional<LLIconCtrl::Params> selected_picture;

		Optional<LLTextBox::Params> text;
		Optional<CommitCallbackParam> mousedown_callback;
		Params();
	};
	/*virtual*/ bool postBuild();
	/** setting on/off background icon to indicate selected state */
	/*virtual*/ void setValue(const LLSD& value);
	// sends commit signal
	void onAnyMouseClick();
protected:
	friend class LLUICtrlFactory;
	LLPanelCameraItem(const Params&);
	LLIconCtrl* mIconOver;
	LLIconCtrl* mIconSelected;
	LLIconCtrl* mPicture;
	LLIconCtrl* mPictureSelected;
	LLTextBox* mText;
};

#endif
