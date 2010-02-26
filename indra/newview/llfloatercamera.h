/** 
 * @file llfloatercamera.h
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

#ifndef LLFLOATERCAMERA_H
#define LLFLOATERCAMERA_H

#include "lltransientdockablefloater.h"

class LLJoystickCameraRotate;
class LLJoystickCameraZoom;
class LLJoystickCameraTrack;
class LLFloaterReg;
class LLPanelCameraZoom;

enum ECameraControlMode
{
	CAMERA_CTRL_MODE_ORBIT,
	CAMERA_CTRL_MODE_PAN,
	CAMERA_CTRL_MODE_FREE_CAMERA,
	CAMERA_CTRL_MODE_AVATAR_VIEW
};

class LLFloaterCamera
	:	public LLTransientDockableFloater
{
	friend class LLFloaterReg;
	
public:

	/* whether in free camera mode */
	static bool inFreeCameraMode();
	/* callback for camera presets changing */
	static void onClickCameraPresets(const LLSD& param);

	static void onLeavingMouseLook();

	/** resets current camera mode to orbit mode */
	static void resetCameraMode();

	/* determines actual mode and updates ui */
	void update();
	
	virtual void onOpen(const LLSD& key);
	virtual void onClose(bool app_quitting);

	LLJoystickCameraRotate* mRotate;
	LLPanelCameraZoom*	mZoom;
	LLJoystickCameraTrack*	mTrack;

private:

	LLFloaterCamera(const LLSD& val);
	~LLFloaterCamera() {};

	/* return instance if it exists - created by LLFloaterReg */
	static LLFloaterCamera* findInstance();

	/*virtual*/ BOOL postBuild();

	ECameraControlMode determineMode();

	/* whether in avatar view (first person) mode */
	bool inAvatarViewMode();

	/* resets to the previous mode */
	void toPrevMode();

	/* sets a new mode and performs related actions */
	void switchMode(ECameraControlMode mode);

	/* sets a new mode preserving previous one and updates ui*/
	void setMode(ECameraControlMode mode);

	/** set title appropriate to passed mode */
	void setModeTitle(const ECameraControlMode mode);

	/* updates the state (UI) according to the current mode */
	void updateState();

	/* update camera preset buttons toggle state according to the currently selected preset */
	void updateCameraPresetButtons();

	void onClickBtn(ECameraControlMode mode);
	void assignButton2Mode(ECameraControlMode mode, const std::string& button_name);
	

	BOOL mClosed;
	ECameraControlMode mPrevMode;
	ECameraControlMode mCurrMode;
	std::map<ECameraControlMode, LLButton*> mMode2Button;

};

#endif
