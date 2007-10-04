/** 
 * @file llagentpilot.cpp
 * @brief LLAgentPilot class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include <iostream>
#include <fstream>
#include <iomanip>

#include "llagentpilot.h"
#include "llagent.h"
#include "llframestats.h"
#include "viewer.h"
#include "llviewercontrol.h"

LLAgentPilot gAgentPilot;

BOOL LLAgentPilot::sLoop = TRUE;

LLAgentPilot::LLAgentPilot()
{
	mRecording = FALSE;
	mPlaying = FALSE;
	mStarted = FALSE;
	mNumRuns = -1;
}

LLAgentPilot::~LLAgentPilot()
{
}

void LLAgentPilot::load(const char *filename)
{
	if(!filename) return;

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

	S32 i;
	for (i = 0; i < num_actions; i++)
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

void LLAgentPilot::save(const char *filename)
{
	llofstream file;
	file.open(filename);			/*Flawfinder: ignore*/

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
	gAgentPilot.save(gSavedSettings.getString("StatsPilotFile").c_str());
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
}

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
						llinfos << "At start, beginning playback" << llendl;
						mTimer.reset();
						LLFrameStats::startLogging(NULL);
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
					LLFrameStats::stopLogging(NULL);
					mNumRuns--;
					if (sLoop)
					{
						if ((mNumRuns < 0) || (mNumRuns > 0))
						{
							llinfos << "Looping, restarting playback" << llendl;
							startPlayback();
						}
						else if (mQuitAfterRuns)
						{
							llinfos << "Done with all runs, quitting viewer!" << llendl;
							app_force_quit(NULL);
						}
						else
						{
							llinfos << "Done with all runs, disabling pilot" << llendl;
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
