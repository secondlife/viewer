/** 
 * @file llagentpilot.h
 * @brief LLAgentPilot class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

	void load(const std::string& filename);
	void save(const std::string& filename);

	void startRecord();
	void stopRecord();
	void addAction(enum EActionType action);

	void startPlayback();
	void stopPlayback();

	void updateTarget();

	static void startRecord(void *);
	static void addWaypoint(void *);
	static void saveRecord(void *);
	static void startPlayback(void *);
	static void stopPlayback(void *);
	static BOOL	sLoop;

	S32		mNumRuns;
	BOOL	mQuitAfterRuns;
private:
	void setAutopilotTarget(const S32 id);

	BOOL	mRecording;
	F32		mLastRecordTime;

	BOOL	mStarted;
	BOOL	mPlaying;
	S32		mCurrentAction;

	class Action
	{
	public:

		EActionType		mType;
		LLVector3d		mTarget;
		F64				mTime;
	};

	LLDynamicArray<Action>	mActions;
	LLTimer					mTimer;
};

extern LLAgentPilot gAgentPilot;

#endif // LL_LLAGENTPILOT_H
