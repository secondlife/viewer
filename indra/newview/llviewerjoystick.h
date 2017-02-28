/** 
 * @file llviewerjoystick.h
 * @brief Viewer joystick / NDOF device functionality.
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

#ifndef LL_LLVIEWERJOYSTICK_H
#define LL_LLVIEWERJOYSTICK_H

#include "stdtypes.h"

#if LIB_NDOF
#include "ndofdev_external.h"
#else
#define NDOF_Device	void
#define NDOF_HotPlugResult S32
#endif

typedef enum e_joystick_driver_state
{
	JDS_UNINITIALIZED,
	JDS_INITIALIZED,
	JDS_INITIALIZING
} EJoystickDriverState;

class LLViewerJoystick : public LLSingleton<LLViewerJoystick>
{
	LLSINGLETON(LLViewerJoystick);
	virtual ~LLViewerJoystick();

public:
	void init(bool autoenable);
	void terminate();

	void updateStatus();
	void scanJoystick();
	void moveObjects(bool reset = false);
	void moveAvatar(bool reset = false);
	void moveFlycam(bool reset = false);
	F32 getJoystickAxis(U32 axis) const;
	U32 getJoystickButton(U32 button) const;
	bool isJoystickInitialized() const {return (mDriverState==JDS_INITIALIZED);}
	bool isLikeSpaceNavigator() const;
	void setNeedsReset(bool reset = true) { mResetFlag = reset; }
	void setCameraNeedsUpdate(bool b)     { mCameraUpdated = b; }
	bool getCameraNeedsUpdate() const     { return mCameraUpdated; }
	bool getOverrideCamera() { return mOverrideCamera; }
	void setOverrideCamera(bool val);
	bool toggleFlycam();
	void setSNDefaults();
	std::string getDescription();
	
protected:
	void updateEnabled(bool autoenable);
	void handleRun(F32 inc);
	void agentSlide(F32 inc);
	void agentPush(F32 inc);
	void agentFly(F32 inc);
	void agentPitch(F32 pitch_inc);
	void agentYaw(F32 yaw_inc);
	void agentJump();
	void resetDeltas(S32 axis[]);
#if LIB_NDOF
	static NDOF_HotPlugResult HotPlugAddCallback(NDOF_Device *dev);
	static void HotPlugRemovalCallback(NDOF_Device *dev);
#endif
	
private:
	F32						mAxes[6];
	long					mBtn[16];
	EJoystickDriverState	mDriverState;
	NDOF_Device				*mNdofDev;
	bool					mResetFlag;
	F32						mPerfScale;
	bool					mCameraUpdated;
	bool 					mOverrideCamera;
	U32						mJoystickRun;
	
	static F32				sLastDelta[7];
	static F32				sDelta[7];
};

#endif
