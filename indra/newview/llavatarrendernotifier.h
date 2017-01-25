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

struct LLHUDComplexity
{
    LLHUDComplexity()
    {
        reset();
    }
    void reset()
    {
        objectId = LLUUID::null;
        objectName = "";
        objectsCost = 0;
        objectsCount = 0;
        texturesCost = 0;
        texturesCount = 0;
        largeTexturesCount = 0;
        texturesMemoryTotal = (F64Bytes)0;
    }
    LLUUID objectId;
    std::string objectName;
    std::string jointName;
    U32 objectsCost;
    U32 objectsCount;
    U32 texturesCost;
    U32 texturesCount;
    U32 largeTexturesCount;
    F64Bytes texturesMemoryTotal;
};

typedef std::list<LLHUDComplexity> hud_complexity_list_t;

// Class to notify user about drastic changes in agent's render weights or if other agents
// reported that user's agent is too 'heavy' for their settings
class LLAvatarRenderNotifier : public LLSingleton<LLAvatarRenderNotifier>
{
public:
	LLAvatarRenderNotifier();

    void displayNotification(bool show_over_limit);
	bool isNotificationVisible();

	void updateNotificationRegion(U32 agentcount, U32 overLimit);
    void updateNotificationState();
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
    std::string overLimitMessage();

    // initial outfit related variables (state control)
    bool mNotifyOutfitLoading;

    // COF (inventory folder) and Skeleton (voavatar) are used to spot changes in outfit.
    S32 mLastCofVersion;
    S32 mLastSkeletonSerialNum;
    // Used to detect changes in voavatar's rezzed status.
    // If value decreases - there were changes in outfit.
    S32 mLastOutfitRezStatus;
};

// Class to notify user about heavy set of HUD
class LLHUDRenderNotifier : public LLSingleton<LLHUDRenderNotifier>
{
public:
    LLHUDRenderNotifier();
    ~LLHUDRenderNotifier();

    void updateNotificationHUD(hud_complexity_list_t complexity);
    bool isNotificationVisible();

private:
    enum EWarnLevel
    {
        WARN_NONE = -1,
        WARN_TEXTURES = 0, // least important
        WARN_CRAMPED,
        WARN_HEAVY,
        WARN_COST,
        WARN_MEMORY, //most important
    };

    LLNotificationPtr mHUDNotificationPtr;

    static EWarnLevel getWarningType(LLHUDComplexity object_complexity, LLHUDComplexity cmp_complexity);
    void displayHUDNotification(EWarnLevel warn_type, LLUUID obj_id = LLUUID::null,  std::string object_name = "", std::string joint_name = "");

    LLHUDComplexity mReportedHUDComplexity;
    EWarnLevel mReportedHUDWarning;
    LLHUDComplexity mLatestHUDComplexity;
    LLFrameTimer mHUDPopUpDelayTimer;
};

#endif /* ! defined(LL_llavatarrendernotifier_H) */
