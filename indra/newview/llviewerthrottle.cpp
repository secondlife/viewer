/** 
 * @file llviewerthrottle.cpp
 * @brief LLViewerThrottle class implementation
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
#include "llviewerthrottle.h"

#include "llviewercontrol.h"
#include "message.h"
#include "llagent.h"
#include "llframetimer.h"
#include "llviewerstats.h"
#include "lldatapacker.h"

// consts

// The viewer is allowed to set the under-the-hood bandwidth to 50%
// greater than the prefs UI shows, under the assumption that the
// viewer won't receive all the different message types at once.
// I didn't design this, don't know who did. JC
const F32 MAX_FRACTIONAL = 1.5f;
const F32 MIN_FRACTIONAL = 0.2f;

const F32 MIN_BANDWIDTH = 50.f;
const F32 MAX_BANDWIDTH = 1500.f;
const F32 STEP_FRACTIONAL = 0.1f;
const F32 TIGHTEN_THROTTLE_THRESHOLD = 3.0f; // packet loss % per s
const F32 EASE_THROTTLE_THRESHOLD = 0.5f; // packet loss % per s
const F32 DYNAMIC_UPDATE_DURATION = 5.0f; // seconds

LLViewerThrottle gViewerThrottle;

// static
const char *LLViewerThrottle::sNames[TC_EOF] = {
							"Resend",
							"Land",
							"Wind",
							"Cloud",
							"Task",
							"Texture",
							"Asset"
							};


// Bandwidth settings for different bit rates, they're interpolated/extrapolated.
//                                Resend Land Wind Cloud Task Texture Asset
const F32 BW_PRESET_50[TC_EOF]   = {   5,  10,   3,   3,  10,  10,   9 };
const F32 BW_PRESET_300[TC_EOF]  = {  30,  40,   9,   9,  86,  86,  40 };
const F32 BW_PRESET_500[TC_EOF]  = {  50,  70,  14,  14, 136, 136,  80 };
const F32 BW_PRESET_1000[TC_EOF] = { 100, 100,  20,  20, 310, 310, 140 };

LLViewerThrottleGroup::LLViewerThrottleGroup()
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		mThrottles[i] = 0.f;
	}
	mThrottleTotal = 0.f;
}


LLViewerThrottleGroup::LLViewerThrottleGroup(const F32 settings[])
{
	mThrottleTotal = 0.f;
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		mThrottles[i] = settings[i];
		mThrottleTotal += settings[i];
	}
}


LLViewerThrottleGroup LLViewerThrottleGroup::operator*(const F32 frac) const
{
	LLViewerThrottleGroup res;
	res.mThrottleTotal = 0.f;

	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		res.mThrottles[i] = mThrottles[i] * frac;
		res.mThrottleTotal += res.mThrottles[i];
	}

	return res;
}


LLViewerThrottleGroup LLViewerThrottleGroup::operator+(const LLViewerThrottleGroup &b) const
{
	LLViewerThrottleGroup res;
	res.mThrottleTotal = 0.f;

	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		res.mThrottles[i] = mThrottles[i] + b.mThrottles[i];
		res.mThrottleTotal += res.mThrottles[i];
	}

	return res;
}


LLViewerThrottleGroup LLViewerThrottleGroup::operator-(const LLViewerThrottleGroup &b) const
{
	LLViewerThrottleGroup res;
	res.mThrottleTotal = 0.f;

	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		res.mThrottles[i] = mThrottles[i] - b.mThrottles[i];
		res.mThrottleTotal += res.mThrottles[i];
	}

	return res;
}


void LLViewerThrottleGroup::sendToSim() const
{
	llinfos << "Sending throttle settings, total BW " << mThrottleTotal << llendl;
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AgentThrottle);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_CircuitCode, msg->mOurCircuitCode);

	msg->nextBlockFast(_PREHASH_Throttle);
	msg->addU32Fast(_PREHASH_GenCounter, 0);

	// Pack up the throttle data
	U8 tmp[64];
	LLDataPackerBinaryBuffer dp(tmp, MAX_THROTTLE_SIZE);
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		//sim wants BPS, not KBPS
		dp.packF32(mThrottles[i] * 1024.0f, "Throttle");
	}
	S32 len = dp.getCurrentSize();
	msg->addBinaryDataFast(_PREHASH_Throttles, tmp, len);

	gAgent.sendReliableMessage();
}


void LLViewerThrottleGroup::dump()
{
	S32 i;
	for (i = 0; i < TC_EOF; i++)
	{
		llinfos << LLViewerThrottle::sNames[i] << ": " << mThrottles[i] << llendl;
	}
	llinfos << "Total: " << mThrottleTotal << llendl;
}

class LLBPSListener : public LLSimpleListener
{
public:
	virtual bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gViewerThrottle.setMaxBandwidth((F32) event->getValue().asReal()*1024);
		return true;
	}
};

LLViewerThrottle::LLViewerThrottle() :
	mMaxBandwidth(0.f),
	mCurrentBandwidth(0.f),
	mThrottleFrac(1.f)
{
	// Need to be pushed on in bandwidth order
	mPresets.push_back(LLViewerThrottleGroup(BW_PRESET_50));
	mPresets.push_back(LLViewerThrottleGroup(BW_PRESET_300));
	mPresets.push_back(LLViewerThrottleGroup(BW_PRESET_500));
	mPresets.push_back(LLViewerThrottleGroup(BW_PRESET_1000));
}


void LLViewerThrottle::setMaxBandwidth(F32 kbits_per_second, BOOL from_event)
{
	if (!from_event)
	{
		gSavedSettings.setF32("ThrottleBandwidthKBPS", kbits_per_second);
	}
	gViewerThrottle.load();

	if (gAgent.getRegion())
	{
		gViewerThrottle.sendToSim();
	}
}

void LLViewerThrottle::load()
{
	mMaxBandwidth = gSavedSettings.getF32("ThrottleBandwidthKBPS")*1024;
	resetDynamicThrottle();
	mCurrent.dump();
}


void LLViewerThrottle::save() const
{
	gSavedSettings.setF32("ThrottleBandwidthKBPS", mMaxBandwidth/1024);
}


void LLViewerThrottle::sendToSim() const
{
	mCurrent.sendToSim();
}


LLViewerThrottleGroup LLViewerThrottle::getThrottleGroup(const F32 bandwidth_kbps)
{
	//Clamp the bandwidth users can set.
	F32 set_bandwidth = llclamp(bandwidth_kbps, MIN_BANDWIDTH, MAX_BANDWIDTH);

	S32 count = mPresets.size();

	S32 i;
	for (i = 0; i < count; i++)
	{
		if (mPresets[i].getTotal() > set_bandwidth)
		{
			break;
		}
	}

	if (i == 0)
	{
		// We return the minimum if it's less than the minimum
		return mPresets[0];
	}
	else if (i == count)
	{
		// Higher than the highest preset, we extrapolate out based on the
		// last two presets.  This allows us to keep certain throttle channels from
		// growing in bandwidth
		F32 delta_bw = set_bandwidth - mPresets[count-1].getTotal();
		LLViewerThrottleGroup delta_throttle = mPresets[count - 1] - mPresets[count - 2];
		F32 delta_total = delta_throttle.getTotal();
		F32 delta_frac = delta_bw / delta_total;
		delta_throttle = delta_throttle * delta_frac;
		return mPresets[count-1] + delta_throttle;
	}
	else
	{
		// In between two presets, just interpolate
		F32 delta_bw = set_bandwidth - mPresets[i - 1].getTotal();
		LLViewerThrottleGroup delta_throttle = mPresets[i] - mPresets[i - 1];
		F32 delta_total = delta_throttle.getTotal();
		F32 delta_frac = delta_bw / delta_total;
		delta_throttle = delta_throttle * delta_frac;
		return mPresets[i - 1] + delta_throttle;
	}
}


// static
void LLViewerThrottle::resetDynamicThrottle()
{
	mThrottleFrac = MAX_FRACTIONAL;

	mCurrentBandwidth = mMaxBandwidth*MAX_FRACTIONAL;
	mCurrent = getThrottleGroup(mCurrentBandwidth / 1024.0f);
}

void LLViewerThrottle::updateDynamicThrottle()
{
	if (mUpdateTimer.getElapsedTimeF32() < DYNAMIC_UPDATE_DURATION)
	{
		return;
	}
	mUpdateTimer.reset();

	if (LLViewerStats::getInstance()->mPacketsLostPercentStat.getMean() > TIGHTEN_THROTTLE_THRESHOLD)
	{
		if (mThrottleFrac <= MIN_FRACTIONAL || mCurrentBandwidth / 1024.0f <= MIN_BANDWIDTH)
		{
			return;
		}
		mThrottleFrac -= STEP_FRACTIONAL;
		mThrottleFrac = llmax(MIN_FRACTIONAL, mThrottleFrac);
		mCurrentBandwidth = mMaxBandwidth * mThrottleFrac;
		mCurrent = getThrottleGroup(mCurrentBandwidth / 1024.0f);
		mCurrent.sendToSim();
		llinfos << "Tightening network throttle to " << mCurrentBandwidth << llendl;
	}
	else if (LLViewerStats::getInstance()->mPacketsLostPercentStat.getMean() <= EASE_THROTTLE_THRESHOLD)
	{
		if (mThrottleFrac >= MAX_FRACTIONAL || mCurrentBandwidth / 1024.0f >= MAX_BANDWIDTH)
		{
			return;
		}
		mThrottleFrac += STEP_FRACTIONAL;
		mThrottleFrac = llmin(MAX_FRACTIONAL, mThrottleFrac);
		mCurrentBandwidth = mMaxBandwidth * mThrottleFrac;
		mCurrent = getThrottleGroup(mCurrentBandwidth/1024.0f);
		mCurrent.sendToSim();
		llinfos << "Easing network throttle to " << mCurrentBandwidth << llendl;
	}
}
