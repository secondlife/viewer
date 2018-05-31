/** 
 * @file llfloaterpreferenceviewadvanced.cpp
 * @brief floater for adjusting camera position
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
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
#include "llfloaterpreferenceviewadvanced.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lluictrlfactory.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"


LLFloaterPreferenceViewAdvanced::LLFloaterPreferenceViewAdvanced(const LLSD& key) 
:	LLFloater(key)
{
	mCommitCallbackRegistrar.add("Cancel",	boost::bind(&LLFloaterPreferenceViewAdvanced::onClickCancel, this));
	mCommitCallbackRegistrar.add("CommitSettings",	boost::bind(&LLFloaterPreferenceViewAdvanced::onCommitSettings, this));
	mCommitCallbackRegistrar.add("Ok",	boost::bind(&LLFloaterPreferenceViewAdvanced::onClickOk, this));

}

LLFloaterPreferenceViewAdvanced::~LLFloaterPreferenceViewAdvanced()
{}

void LLFloaterPreferenceViewAdvanced::onClickOk()
{
	closeFloater();
}

void LLFloaterPreferenceViewAdvanced::onClickCancel()
{
	gSavedSettings.setVector3("CameraOffsetRearView", mCameraSaved);
	gSavedSettings.setVector3d("FocusOffsetRearView", mFocusSaved);

	updateCameraControl(mCameraSaved);
	updateFocusControl(mFocusSaved);
}

BOOL LLFloaterPreferenceViewAdvanced::postBuild()
{
	mCameraSaved = gSavedSettings.getVector3("CameraOffsetRearView");
	mFocusSaved = gSavedSettings.getVector3d("FocusOffsetRearView");

	updateCameraControl(mCameraSaved);
	updateFocusControl(mFocusSaved);

	return TRUE;
}

void LLFloaterPreferenceViewAdvanced::updateCameraControl(const LLVector3& vector)
{
	getChild<LLSpinCtrl>("camera_x")->setValue(vector[VX]);
	getChild<LLSpinCtrl>("camera_y")->setValue(vector[VY]);
	getChild<LLSpinCtrl>("camera_z")->setValue(vector[VZ]);
}

void LLFloaterPreferenceViewAdvanced::updateFocusControl(const LLVector3d& vector3d)
{
	getChild<LLSpinCtrl>("focus_x")->setValue(vector3d[VX]);
	getChild<LLSpinCtrl>("focus_y")->setValue(vector3d[VY]);
	getChild<LLSpinCtrl>("focus_z")->setValue(vector3d[VZ]);
}

 void LLFloaterPreferenceViewAdvanced::draw()
{
	static LLCachedControl<LLVector3> camera(gSavedSettings, "CameraOffsetRearView");
	static LLCachedControl<LLVector3d> focus(gSavedSettings, "FocusOffsetRearView");

	updateCameraControl(camera);
	updateFocusControl(focus);

	LLFloater::draw();
}

void LLFloaterPreferenceViewAdvanced::onCommitSettings()
{
	LLVector3 vector;
	LLVector3d vector3d;

	vector.mV[VX] = (F32)getChild<LLUICtrl>("camera_x")->getValue().asReal();
	vector.mV[VY] = (F32)getChild<LLUICtrl>("camera_y")->getValue().asReal();
	vector.mV[VZ] = (F32)getChild<LLUICtrl>("camera_z")->getValue().asReal();
	gSavedSettings.setVector3("CameraOffsetRearView", vector);

	vector3d.mdV[VX] = (F32)getChild<LLUICtrl>("focus_x")->getValue().asReal();
	vector3d.mdV[VY] = (F32)getChild<LLUICtrl>("focus_y")->getValue().asReal();
	vector3d.mdV[VZ] = (F32)getChild<LLUICtrl>("focus_z")->getValue().asReal();
	gSavedSettings.setVector3d("FocusOffsetRearView", vector3d);
}
