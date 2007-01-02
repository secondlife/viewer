/** 
 * @file llagentpilot.h
 * @brief LLAgentPilot class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	void load(const char *filename);
	void save(const char *filename);

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
