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
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llnotificationtemplate.h"
#include "lltimer.h"
#include "llvoavatarself.h"
#include "llviewercontrol.h"
#include "lltrans.h"
#include "llagentcamera.h"
// associated header
#include "llavatarrendernotifier.h"

// when change exceeds this ration, notification is shown
static const F32 RENDER_ALLOWED_CHANGE_PCT = 0.1;
// wait seconds before processing over limit updates after last complexity change
static const U32 OVER_LIMIT_UPDATE_DELAY = 70;

static const U32 WARN_HUD_OBJECTS_LIMIT = 1000;
static const U32 WARN_HUD_TEXTURES_LIMIT = 200;
static const U32 WARN_HUD_OVERSIZED_TEXTURES_LIMIT = 6;
static const U32 WARN_HUD_TEXTURE_MEMORY_LIMIT = 10000000; // in pixels


LLAvatarRenderNotifier::LLAvatarRenderNotifier() :
mAgentsCount(0),
mOverLimitAgents(0),
mAgentComplexity(0),
mOverLimitPct(0.0f),
mLatestAgentsCount(0),
mLatestOverLimitAgents(0),
mLatestAgentComplexity(0),
mLatestOverLimitPct(0.0f),
mShowOverLimitAgents(false),
mNotifyOutfitLoading(false),
mLastCofVersion(LLViewerInventoryCategory::VERSION_UNKNOWN),
mLastOutfitRezStatus(-1),
mLastSkeletonSerialNum(-1)
{
    mPopUpDelayTimer.resetWithExpiry(OVER_LIMIT_UPDATE_DELAY);
}

std::string LLAvatarRenderNotifier::overLimitMessage()
{
    static const char* everyone_now = "av_render_everyone_now";
    static const char* not_everyone = "av_render_not_everyone";
    static const char* over_half = "av_render_over_half";
    static const char* most = "av_render_most_of";
    static const char* anyone = "av_render_anyone";

    std::string message;
    if ( mLatestOverLimitPct >= 99.0 )
    {
        message = anyone;
    }
    else if ( mLatestOverLimitPct >= 75.0 )
    {
        message = most;
    }
    else if ( mLatestOverLimitPct >= 50.0 )
    {
        message = over_half;
    }
    else if ( mLatestOverLimitPct > 10.0 )
    {
        message = not_everyone;
    }
    else
    {
        // Will be shown only after overlimit was > 0
        message = everyone_now;
    }
    return LLTrans::getString(message);
}

void LLAvatarRenderNotifier::displayNotification(bool show_over_limit)
{
    mAgentComplexity = mLatestAgentComplexity;
    mShowOverLimitAgents = show_over_limit;
	static LLCachedControl<U32> expire_delay(gSavedSettings, "ShowMyComplexityChanges", 20);

	LLDate expire_date(LLDate::now().secondsSinceEpoch() + expire_delay);
	LLSD args;
	args["AGENT_COMPLEXITY"] = LLSD::Integer(mLatestAgentComplexity);
	std::string notification_name;
    if (mShowOverLimitAgents)
    {
        notification_name = "AgentComplexityWithVisibility";
        args["OVERLIMIT_MSG"] = overLimitMessage();

        // remember what the situation was so that we only notify when it has changed
        mAgentsCount = mLatestAgentsCount;
        mOverLimitAgents = mLatestOverLimitAgents;
        mOverLimitPct = mLatestOverLimitPct;
	}
	else
	{
        // no change in visibility, just update complexity
        notification_name = "AgentComplexity";
	}

	if (mNotificationPtr != NULL && mNotificationPtr->getName() != notification_name)
	{
		// since unique tag works only for same notification,
		// old notification needs to be canceled manually
		LLNotifications::instance().cancel(mNotificationPtr);
	}

    // log unconditionally
    LL_WARNS("AvatarRenderInfo") << notification_name << " " << args << LL_ENDL;

    if (   expire_delay // expiration of zero means do not show the notices
        && gAgentCamera.getLastCameraMode() != CAMERA_MODE_MOUSELOOK // don't display notices in Mouselook
        )
    {
        mNotificationPtr = LLNotifications::instance().add(LLNotification::Params()
                                                           .name(notification_name)
                                                           .expiry(expire_date)
                                                           .substitutions(args));
    }
}

bool LLAvatarRenderNotifier::isNotificationVisible()
{
	return mNotificationPtr != NULL && mNotificationPtr->isActive();
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

    if (mAgentsCount == mLatestAgentsCount
        && mOverLimitAgents == mLatestOverLimitAgents)
    {
        // no changes since last notification
        return;
    }

    if ((mPopUpDelayTimer.hasExpired() || (isNotificationVisible() && mShowOverLimitAgents))
        && (mOverLimitPct > 0 || mLatestOverLimitPct > 0)
        && std::abs(mOverLimitPct - mLatestOverLimitPct) > mLatestOverLimitPct * RENDER_ALLOWED_CHANGE_PCT
        )
    {
        // display in case of drop to/from zero and in case of significant (RENDER_ALLOWED_CHANGE_PCT) changes
        displayNotification(true);

        // default timeout before next notification
        static LLCachedControl<U32> pop_up_delay(gSavedSettings, "ComplexityChangesPopUpDelay", 300);
        mPopUpDelayTimer.resetWithExpiry(pop_up_delay);
    }
}

void LLAvatarRenderNotifier::updateNotificationState()
{
    if (!isAgentAvatarValid())
    {
        // data not ready, nothing to show.
        return;
    }

    // Don't use first provided COF and Sceleton versions - let them load anf 'form' first
    if (mLastCofVersion < 0
        && gAgentWearables.areWearablesLoaded()
        && LLAttachmentsMgr::getInstance()->isAttachmentStateComplete())
    {
        // cof formed
        mLastCofVersion = LLAppearanceMgr::instance().getCOFVersion();
        mLastSkeletonSerialNum = gAgentAvatarp->mLastSkeletonSerialNum;
    }
    else if (mLastCofVersion >= 0
        && (mLastCofVersion != LLAppearanceMgr::instance().getCOFVersion()
        || mLastSkeletonSerialNum != gAgentAvatarp->mLastSkeletonSerialNum))
    {
        // version mismatch in comparison to previous outfit - outfit changed
        mNotifyOutfitLoading = true;
        mLastCofVersion = LLAppearanceMgr::instance().getCOFVersion();
        mLastSkeletonSerialNum = gAgentAvatarp->mLastSkeletonSerialNum;
    }

    if (gAgentAvatarp->mLastRezzedStatus < mLastOutfitRezStatus)
    {
        // rez status decreased - outfit related action was initiated
        mNotifyOutfitLoading = true;
    }

    mLastOutfitRezStatus = gAgentAvatarp->mLastRezzedStatus;
}
void LLAvatarRenderNotifier::updateNotificationAgent(U32 agentComplexity)
{
    // save the value for use in following messages
    mLatestAgentComplexity = agentComplexity;

    if (!isAgentAvatarValid() || !gAgentWearables.areWearablesLoaded())
    {
        // data not ready, nothing to show.
        return;
    }

    if (!mNotifyOutfitLoading)
    {
        // We should not notify about initial outfit and it's load process without reason
        updateNotificationState();

        if (mLatestOverLimitAgents > 0)
        {
            // Some users can't see agent already, notify user about complexity growth
            mNotifyOutfitLoading = true;
        }

        if (!mNotifyOutfitLoading)
        {
            // avatar or outfit not ready
            mAgentComplexity = mLatestAgentComplexity;
            return;
        }
    }

    if (mAgentComplexity != mLatestAgentComplexity)
    {
        // if we have an agent complexity change, we always display it and hide 'over limit'
        displayNotification(false);

        // next 'over limit' update should be displayed after delay to make sure information got updated at server side
        mPopUpDelayTimer.resetWithExpiry(OVER_LIMIT_UPDATE_DELAY);
    }
}

// LLHUDRenderNotifier

LLHUDRenderNotifier::LLHUDRenderNotifier()
{
}

LLHUDRenderNotifier::~LLHUDRenderNotifier()
{
}

void LLHUDRenderNotifier::updateNotificationHUD(LLHUDComplexity new_complexity)
{
    if (!isAgentAvatarValid())
    {
        // data not ready.
        return;
    }

    static const char* hud_memory = "hud_render_memory_warning";
    static const char* hud_cost = "hud_render_cost_warning";
    static const char* hud_heavy = "hud_render_heavy_textures_warning";
    static const char* hud_cramped = "hud_render_cramped_warning";
    static const char* hud_textures = "hud_render_textures_warning";

    static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0U); // ties max HUD cost to avatar cost
    static LLCachedControl<U32> max_objects_count(gSavedSettings, "RenderHUDObjectsWarning", WARN_HUD_OBJECTS_LIMIT);
    static LLCachedControl<U32> max_textures_count(gSavedSettings, "RenderHUDTexturesWarning", WARN_HUD_TEXTURES_LIMIT);
    static LLCachedControl<U32> max_oversized_count(gSavedSettings, "RenderHUDOversizedTexturesWarning", WARN_HUD_OVERSIZED_TEXTURES_LIMIT);
    static LLCachedControl<U32> max_texture_memory(gSavedSettings, "RenderHUDTexturesVirtualMemoryWarning", WARN_HUD_TEXTURE_MEMORY_LIMIT);

    if (mHUDPopUpDelayTimer.hasExpired())
    {
        // Show warning with highest importance (5m delay between warnings by default)
        // TODO:
        // Consider showing message with list of issues.
        // For now shows one after another if update arrives and timer expired, so
        // consider showing only one most important or consider triggering not
        // only in case of update
        if (mReportedHUDComplexity.texturesSizeTotal < new_complexity.texturesSizeTotal
            && new_complexity.texturesSizeTotal > max_texture_memory)
        {
            displayHUDNotification(hud_memory);
            LL_DEBUGS("HUDdetail") << "HUD memory usage over limit,"
                                   << " was " << mReportedHUDComplexity.texturesSizeTotal
                                   << " is " << new_complexity.texturesSizeTotal << LL_ENDL;
            mReportedHUDComplexity.texturesSizeTotal = new_complexity.texturesSizeTotal;
        }
        else if ((mReportedHUDComplexity.objectsCost < new_complexity.objectsCost
            || mReportedHUDComplexity.texturesCost < new_complexity.texturesCost)
            && max_render_cost > 0
            && new_complexity.objectsCost + new_complexity.texturesCost > max_render_cost)
        {
            LL_DEBUGS("HUDdetail") << "HUD complexity over limit,"
                                   << " HUD textures cost: " << new_complexity.texturesCost
                                   << " HUD objects cost: " << new_complexity.objectsCost << LL_ENDL;
            displayHUDNotification(hud_cost);
            mReportedHUDComplexity.objectsCost = new_complexity.objectsCost;
            mReportedHUDComplexity.texturesCost = new_complexity.texturesCost;
        }
        else if (mReportedHUDComplexity.largeTexturesCount < new_complexity.largeTexturesCount
            && new_complexity.largeTexturesCount > max_oversized_count)
        {
            LL_DEBUGS("HUDdetail") << "HUD contains to many large textures: "
                                   << new_complexity.largeTexturesCount << LL_ENDL;
            displayHUDNotification(hud_heavy);
            mReportedHUDComplexity.largeTexturesCount = new_complexity.largeTexturesCount;
        }
        else if (mReportedHUDComplexity.texturesCount < new_complexity.texturesCount
            && new_complexity.texturesCount > max_textures_count)
        {
            LL_DEBUGS("HUDdetail") << "HUD contains too many textures: "
                                   << new_complexity.texturesCount << LL_ENDL;
            displayHUDNotification(hud_cramped);
            mReportedHUDComplexity.texturesCount = new_complexity.texturesCount;
        }
        else if (mReportedHUDComplexity.objectsCount < new_complexity.objectsCount
            && new_complexity.objectsCount > max_objects_count)
        {
            LL_DEBUGS("HUDdetail") << "HUD contains too many objects: "
                                   << new_complexity.objectsCount << LL_ENDL;
            displayHUDNotification(hud_textures);
            mReportedHUDComplexity.objectsCount = new_complexity.objectsCount;
        }
        else
        {
            // all warnings displayed, just store everything so that we will
            // be able to reduce values and show warnings again later
            mReportedHUDComplexity = new_complexity;
        }
    }

    if (mLatestHUDComplexity.objectsCost != new_complexity.objectsCost
        || mLatestHUDComplexity.objectsCount != new_complexity.objectsCount
        || mLatestHUDComplexity.texturesCost != new_complexity.texturesCost
        || mLatestHUDComplexity.texturesCount != new_complexity.texturesCount
        || mLatestHUDComplexity.largeTexturesCount != new_complexity.largeTexturesCount
        || mLatestHUDComplexity.texturesSizeTotal != new_complexity.texturesSizeTotal)
    {
        LL_INFOS("HUDdetail") << "HUD textures count: " << new_complexity.texturesCount
            << " HUD textures cost: " << new_complexity.texturesCost
            << " Large textures: " << new_complexity.largeTexturesCount
            << " HUD objects cost: " << new_complexity.objectsCost
            << " HUD objects count: " << new_complexity.objectsCount << LL_ENDL;

        mLatestHUDComplexity = new_complexity;
    }
    
}

void LLHUDRenderNotifier::displayHUDNotification(const char* message)
{
    static LLCachedControl<U32> pop_up_delay(gSavedSettings, "ComplexityChangesPopUpDelay", 300);
    static LLCachedControl<U32> expire_delay(gSavedSettings, "ShowMyComplexityChanges", 20);
    LLDate expire_date(LLDate::now().secondsSinceEpoch() + expire_delay);

    LLSD args;
    args["HUD_REASON"] = LLTrans::getString(message);

    LLNotifications::instance().add(LLNotification::Params()
        .name("HUDComplexityWarning")
        .expiry(expire_date)
        .substitutions(args));
    mHUDPopUpDelayTimer.resetWithExpiry(pop_up_delay);
}

