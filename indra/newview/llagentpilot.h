/** 
 * @file llagentpilot.h
 * @brief LLAgentPilot class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLAGENTPILOT_H
#define LL_LLAGENTPILOT_H

#include "stdtypes.h"
#include "lltimer.h"
#include "v3dmath.h"
#include "lldarray.h"

// Class that drives the agent around according to a "script".

class LLAgentPilot
{
public:
	enum EActionType
	{
		STRAIGHT,
		TURN
	};

	LLAgentPilot();
	virtual ~LLAgentPilot();

	void load();
	void loadTxt(const std::string& filename);
	void loadXML(const std::string& filename);
	void save();
	void saveTxt(const std::string& filename);
	void saveXML(const std::string& filename);

	void startRecord();
	void stopRecord();
	void addAction(enum EActionType action);

	void startPlayback();
	void stopPlayback();

	bool isRecording() { return mRecording; }
	bool isPlaying() { return mPlaying; }
	bool getOverrideCamera() { return mOverrideCamera; }
	
	void updateTarget();

	void addWaypoint();
	void moveCamera();

	void setReplaySession(BOOL new_val) { mReplaySession = new_val; }
	BOOL getReplaySession() { return mReplaySession; }

	void setLoop(BOOL new_val) { mLoop = new_val; }
	BOOL getLoop() { return mLoop; }

	void setQuitAfterRuns(BOOL quit_val) { mQuitAfterRuns = quit_val; }
	void setNumRuns(S32 num_runs) { mNumRuns = num_runs; }
	
private:



	BOOL	mLoop;
	BOOL 	mReplaySession;

	S32		mNumRuns;
	BOOL	mQuitAfterRuns;

	void setAutopilotTarget(const S32 id);

	BOOL	mRecording;
	F32		mLastRecordTime;

	BOOL	mStarted;
	BOOL	mPlaying;
	S32		mCurrentAction;

	BOOL	mOverrideCamera;

	class Action
	{
	public:

		EActionType		mType;
		LLVector3d		mTarget;
		F64				mTime;
		F32				mCameraView;
		LLVector3		mCameraOrigin;
		LLVector3		mCameraXAxis;
		LLVector3		mCameraYAxis;
		LLVector3		mCameraZAxis;
	};

	LLDynamicArray<Action>	mActions;
	LLTimer					mTimer;

};

extern LLAgentPilot gAgentPilot;

#endif // LL_LLAGENTPILOT_H
