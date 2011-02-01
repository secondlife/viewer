/** 
 * @file llagentpilot.cpp
 * @brief LLAgentPilot class implementation
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

#include "llviewerprecompiledheaders.h"

#include <iostream>
#include <fstream>
#include <iomanip>

#include "llagentpilot.h"
#include "llagent.h"
#include "llappviewer.h"
#include "llviewercontrol.h"

LLAgentPilot gAgentPilot;

BOOL LLAgentPilot::sLoop = TRUE;
BOOL LLAgentPilot::sReplaySession = FALSE;

LLAgentPilot::LLAgentPilot() :
	mNumRuns(-1),
	mQuitAfterRuns(FALSE),
	mRecording(FALSE),
	mLastRecordTime(0.f),
	mStarted(FALSE),
	mPlaying(FALSE),
	mCurrentAction(0)
{
}

LLAgentPilot::~LLAgentPilot()
{
}

void LLAgentPilot::load(const std::string& filename)
{
	if(filename.empty())
	{
		return;
	}
	
	llifstream file(filename);

	if (!file)
	{
		lldebugs << "Couldn't open " << filename
			<< ", aborting agentpilot load!" << llendl;
		return;
	}
	else
	{
		llinfos << "Opening pilot file " << filename << llendl;
	}

	S32 num_actions;

	file >> num_actions;

	for (S32 i = 0; i < num_actions; i++)
	{
		S32 action_type;
		Action new_action;
		file >> new_action.mTime >> action_type;
		file >> new_action.mTarget.mdV[VX] >> new_action.mTarget.mdV[VY] >> new_action.mTarget.mdV[VZ];
		new_action.mType = (EActionType)action_type;
		mActions.put(new_action);
	}

	file.close();
}

void LLAgentPilot::save(const std::string& filename)
{
	llofstream file;
	file.open(filename);

	if (!file)
	{
		llinfos << "Couldn't open " << filename << ", aborting agentpilot save!" << llendl;
	}

	file << mActions.count() << '\n';

	S32 i;
	for (i = 0; i < mActions.count(); i++)
	{
		file << mActions[i].mTime << "\t" << mActions[i].mType << "\t";
		file << std::setprecision(32) << mActions[i].mTarget.mdV[VX] << "\t" << mActions[i].mTarget.mdV[VY] << "\t" << mActions[i].mTarget.mdV[VZ] << '\n';
	}

	file.close();
}

void LLAgentPilot::startRecord()
{
	mActions.reset();
	mTimer.reset();
	addAction(STRAIGHT);
	mRecording = TRUE;
}

void LLAgentPilot::stopRecord()
{
	gAgentPilot.addAction(STRAIGHT);
	gAgentPilot.save(gSavedSettings.getString("StatsPilotFile"));
	mRecording = FALSE;
}

void LLAgentPilot::addAction(enum EActionType action_type)
{
	llinfos << "Adding waypoint: " << gAgent.getPositionGlobal() << llendl;
	Action action;
	action.mType = action_type;
	action.mTarget = gAgent.getPositionGlobal();
	action.mTime = mTimer.getElapsedTimeF32();
	mLastRecordTime = (F32)action.mTime;
	mActions.put(action);
}

void LLAgentPilot::startPlayback()
{
	if (!mPlaying)
	{
		mPlaying = TRUE;
		mCurrentAction = 0;
		mTimer.reset();

		if (mActions.count())
		{
			llinfos << "Starting playback, moving to waypoint 0" << llendl;
			gAgent.startAutoPilotGlobal(mActions[0].mTarget);
			mStarted = FALSE;
		}
		else
		{
			llinfos << "No autopilot data, cancelling!" << llendl;
			mPlaying = FALSE;
		}
	}
}

void LLAgentPilot::stopPlayback()
{
	if (mPlaying)
	{
		mPlaying = FALSE;
		mCurrentAction = 0;
		mTimer.reset();
		gAgent.stopAutoPilot();
	}

	if (sReplaySession)
	{
		LLAppViewer::instance()->forceQuit();
	}
}

#define SKIP_PILOT_LOGGING 1

void LLAgentPilot::updateTarget()
{
	if (mPlaying)
	{
		if (mCurrentAction < mActions.count())
		{
			if (0 == mCurrentAction)
			{
				if (gAgent.getAutoPilot())
				{
					// Wait until we get to the first location before starting.
					return;
				}
				else
				{
					if (!mStarted)
					{
#if SKIP_PILOT_LOGGING
						llinfos << "At start, beginning playback" << llendl;
#endif
						mTimer.reset();
						mStarted = TRUE;
					}
				}
			}
			if (mTimer.getElapsedTimeF32() > mActions[mCurrentAction].mTime)
			{
				//gAgent.stopAutoPilot();
				mCurrentAction++;

				if (mCurrentAction < mActions.count())
				{
					gAgent.startAutoPilotGlobal(mActions[mCurrentAction].mTarget);
				}
				else
				{
					stopPlayback();
					mNumRuns--;
					if (sLoop)
					{
						if ((mNumRuns < 0) || (mNumRuns > 0))
						{
#if SKIP_PILOT_LOGGING
							llinfos << "Looping, restarting playback" << llendl;
#endif
							startPlayback();
						}
						else if (mQuitAfterRuns)
						{
#if SKIP_PILOT_LOGGING
							llinfos << "Done with all runs, quitting viewer!" << llendl;
#endif
							LLAppViewer::instance()->forceQuit();
						}
						else
						{
#if SKIP_PILOT_LOGGING
							llinfos << "Done with all runs, disabling pilot" << llendl;
#endif
							stopPlayback();
						}
					}
				}
			}
		}
		else
		{
			stopPlayback();
		}
	}
	else if (mRecording)
	{
		if (mTimer.getElapsedTimeF32() - mLastRecordTime > 1.f)
		{
			addAction(STRAIGHT);
		}
	}
}

// static
void LLAgentPilot::startRecord(void *)
{
	gAgentPilot.startRecord();
}

void LLAgentPilot::saveRecord(void *)
{
	gAgentPilot.stopRecord();
}

void LLAgentPilot::addWaypoint(void *)
{
	gAgentPilot.addAction(STRAIGHT);
}

void LLAgentPilot::startPlayback(void *)
{
	gAgentPilot.mNumRuns = -1;
	gAgentPilot.startPlayback();
}

void LLAgentPilot::stopPlayback(void *)
{
	gAgentPilot.stopPlayback();
}
