/** 
 * @file llviewerjoystick.h
 * @brief Viewer joystick / NDOF device functionality.
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
public:
	LLViewerJoystick();
	virtual ~LLViewerJoystick();
	
	void init(bool autoenable);
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
	void terminate();
	void handleRun(F32 inc);
	void agentSlide(F32 inc);
	void agentPush(F32 inc);
	void agentFly(F32 inc);
	void agentRotate(F32 pitch_inc, F32 turn_inc);
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
