/**
 * @file   llavatarrendernotifier.cpp
 * @author andreykproductengine
 * @date   2015-08-05
 * @brief  
 * 
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

// Pre-compiled headers
#include "llviewerprecompiledheaders.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llagentwearables.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationtemplate.h"
#include "lltimer.h"
#include "llviewercontrol.h"
// associated header
#include "llavatarrendernotifier.h"

// when change exceeds this ration, notification is shown
static const F32 RENDER_ALLOWED_CHANGE_PCT = 0.1;


LLAvatarRenderNotifier::LLAvatarRenderNotifier() :
mAgentsCount(0),
mOverLimitAgents(0),
mAgentComplexity(0),
mOverLimitPct(0.0f),
mLatestAgentsCount(0),
mLatestOverLimitAgents(0),
mLatestAgentComplexity(0),
mLatestOverLimitPct(0.0f),
mShowOverLimitAgents(false)
{
}

void LLAvatarRenderNotifier::displayNotification()
{
	static LLCachedControl<U32> expire_delay(gSavedSettings, "ShowMyComplexityChanges", 20);

	LLDate expire_date(LLDate::now().secondsSinceEpoch() + expire_delay);
	LLSD args;
	args["AGENT_COMPLEXITY"] = LLSD::Integer(mLatestAgentComplexity);
	std::string notification_name;
	if (mShowOverLimitAgents)
	{
		notification_name = "RegionAndAgentComplexity";
		args["OVERLIMIT_PCT"] = LLSD::Integer(mLatestOverLimitPct);
	}
	else
	{
		notification_name = "AgentComplexity";
	}

	if (mNotificationPtr != NULL && mNotificationPtr->getName() != notification_name)
	{
		// since unique tag works only for same notification,
		// old notification needs to be canceled manually
		LLNotifications::instance().cancel(mNotificationPtr);
	}

	mNotificationPtr = LLNotifications::instance().add(LLNotification::Params()
		.name(notification_name)
		.expiry(expire_date)
		.substitutions(args));
}

bool LLAvatarRenderNotifier::isNotificationVisible()
{
	return mNotificationPtr != NULL && mNotificationPtr->isActive();
}

void LLAvatarRenderNotifier::updateNotification()
{
	if (mAgentsCount == mLatestAgentsCount
		&& mOverLimitAgents == mLatestOverLimitAgents
		&& mAgentComplexity == mLatestAgentComplexity)
	{
		//no changes since last notification
		return;
	}

	if (mLatestAgentComplexity == 0
		|| !gAgentWearables.areWearablesLoaded())
	{
		// data not ready, nothing to show.
		return;
	}

	bool display_notification = false;
	bool is_visible = isNotificationVisible();

	if (mLatestOverLimitPct > 0 || mOverLimitPct > 0)
	{
		//include 'over limit' information into notification
		mShowOverLimitAgents = true;
	}
	else
	{
		// make sure that 'over limit' won't be displayed only to be hidden in a second
		mShowOverLimitAgents &= is_visible;
	}

	if (mAgentComplexity != mLatestAgentComplexity)
	{
		// if we have an agent complexity update, we always display it 
		display_notification = true;

		// next 'over limit' update should be displayed as soon as possible if there is anything noteworthy
		mPopUpDelayTimer.resetWithExpiry(0);
	}
	else if ((mPopUpDelayTimer.hasExpired() || is_visible)
		&& (mOverLimitPct > 0 || mLatestOverLimitPct > 0)
		&& abs(mOverLimitPct - mLatestOverLimitPct) > mLatestOverLimitPct * RENDER_ALLOWED_CHANGE_PCT)
	{
		// display in case of drop to/from zero and in case of significant (RENDER_ALLOWED_CHANGE_PCT) changes
		display_notification = true;

		// default timeout before next notification
		static LLCachedControl<U32> pop_up_delay(gSavedSettings, "ComplexityChangesPopUpDelay", 300);
		mPopUpDelayTimer.resetWithExpiry(pop_up_delay);
	}

	if (display_notification)
	{
		mAgentComplexity = mLatestAgentComplexity;
		mAgentsCount = mLatestAgentsCount;
		mOverLimitAgents = mLatestOverLimitAgents;
		mOverLimitPct = mLatestOverLimitPct;

		displayNotification();
	}
}

void LLAvatarRenderNotifier::updateNotificationRegion(U32 agentcount, U32 overLimit)
{
	if (agentcount == 0)
	{
		// Data not ready
		return;
	}

	// save current values for later use
	mLatestAgentsCount = agentcount > overLimit ? agentcount - 1 : agentcount; // subtract self
	mLatestOverLimitAgents = overLimit;
	mLatestOverLimitPct = mLatestAgentsCount != 0 ? ((F32)overLimit / (F32)mLatestAgentsCount) * 100.0 : 0;

	updateNotification();
}

void LLAvatarRenderNotifier::updateNotificationAgent(U32 agentComplexity)
{
	// save the value for use in following messages
	mLatestAgentComplexity = agentComplexity;

	updateNotification();
}

