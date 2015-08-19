/**
 * @file   llavatarrendernotifier.h
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

#if ! defined(LL_llavatarrendernotifier_H)
#define LL_llavatarrendernotifier_H

#include "llnotificationptr.h"

class LLViewerRegion;

// Class to notify user about drastic changes in agent's render weights or if other agents
// reported that user's agent is too 'heavy' for their settings
class LLAvatarRenderNotifier : public LLSingleton<LLAvatarRenderNotifier>
{
public:
	LLAvatarRenderNotifier();

	void displayNotification();
	bool isNotificationVisible();

	void updateNotification();
	void updateNotificationRegion(U32 agentcount, U32 overLimit);
	void updateNotificationAgent(U32 agentComplexity);

private:

	LLNotificationPtr mNotificationPtr;

	// to prevent notification from popping up too often, show it only
	// if certain amount of time passed since previous notification
	LLFrameTimer mPopUpDelayTimer;

	// values since last notification for comparison purposes
	U32 mAgentsCount;
	U32 mOverLimitAgents;
	U32 mAgentComplexity;
	F32 mOverLimitPct;

	// last reported values
	U32 mLatestAgentsCount;
	U32 mLatestOverLimitAgents;
	U32 mLatestAgentComplexity;
	F32 mLatestOverLimitPct;

	bool mShowOverLimitAgents;
};

#endif /* ! defined(LL_llavatarrendernotifier_H) */
