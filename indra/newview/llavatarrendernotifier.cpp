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
#include "llslurl.h"
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
static const U32 WARN_HUD_TEXTURE_MEMORY_LIMIT = 32000000; // in bytes


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

static const char* e_hud_messages[] =
{
    "hud_render_textures_warning",
    "hud_render_cramped_warning",
    "hud_render_heavy_textures_warning",
    "hud_render_cost_warning",
    "hud_render_memory_warning",
};

LLHUDRenderNotifier::LLHUDRenderNotifier() :
mReportedHUDWarning(WARN_NONE)
{
}

LLHUDRenderNotifier::~LLHUDRenderNotifier()
{
}

void LLHUDRenderNotifier::updateNotificationHUD(hud_complexity_list_t complexity)
{
    if (!isAgentAvatarValid() || !gAgentWearables.areWearablesLoaded())
    {
        // data not ready.
        return;
    }

    // TODO:
    // Find a way to show message with list of issues, but without making it too large
    // and intrusive.

    LLHUDComplexity new_total_complexity;
    LLHUDComplexity report_complexity;

    hud_complexity_list_t::iterator iter = complexity.begin();
    hud_complexity_list_t::iterator end = complexity.end();
    EWarnLevel warning_level = WARN_NONE;
    for (; iter != end; ++iter)
    {
        LLHUDComplexity object_complexity = *iter;
        EWarnLevel object_level = getWarningType(object_complexity, report_complexity);
        if (object_level >= 0)
        {
            warning_level = object_level;
            report_complexity = object_complexity;
        }
        new_total_complexity.objectsCost += object_complexity.objectsCost;
        new_total_complexity.objectsCount += object_complexity.objectsCount;
        new_total_complexity.texturesCost += object_complexity.texturesCost;
        new_total_complexity.texturesCount += object_complexity.texturesCount;
        new_total_complexity.largeTexturesCount += object_complexity.largeTexturesCount;
        new_total_complexity.texturesMemoryTotal += object_complexity.texturesMemoryTotal;
    }

    if (mHUDPopUpDelayTimer.hasExpired() || isNotificationVisible())
    {
        if (warning_level >= 0)
        {
            // Display info about most complex HUD object
            // make sure it shown only once unless object's complexity or object itself changed
            if (mReportedHUDComplexity.objectId != report_complexity.objectId
                || mReportedHUDWarning != warning_level)
            {
                displayHUDNotification(warning_level, report_complexity.objectId, report_complexity.objectName, report_complexity.jointName);
                mReportedHUDComplexity = report_complexity;
                mReportedHUDWarning = warning_level;
            }
        }
        else
        {
            // Check if total complexity is above threshold and above previous warning
            // Show warning with highest importance (5m delay between warnings by default)
            if (!mReportedHUDComplexity.objectId.isNull())
            {
                mReportedHUDComplexity.reset();
                mReportedHUDWarning = WARN_NONE;
            }

            warning_level = getWarningType(new_total_complexity, mReportedHUDComplexity);
            if (warning_level >= 0 && mReportedHUDWarning != warning_level)
            {
                displayHUDNotification(warning_level);
            }
            mReportedHUDComplexity = new_total_complexity;
            mReportedHUDWarning = warning_level;
        }
    }
    else if (warning_level >= 0)
    {
        LL_DEBUGS("HUDdetail") << "HUD individual warning postponed" << LL_ENDL;
    }

    if (mLatestHUDComplexity.objectsCost != new_total_complexity.objectsCost
        || mLatestHUDComplexity.objectsCount != new_total_complexity.objectsCount
        || mLatestHUDComplexity.texturesCost != new_total_complexity.texturesCost
        || mLatestHUDComplexity.texturesCount != new_total_complexity.texturesCount
        || mLatestHUDComplexity.largeTexturesCount != new_total_complexity.largeTexturesCount
        || mLatestHUDComplexity.texturesMemoryTotal != new_total_complexity.texturesMemoryTotal)
    {
        LL_INFOS("HUDdetail") << "HUD textures count: " << new_total_complexity.texturesCount
            << " HUD textures cost: " << new_total_complexity.texturesCost
            << " Large textures: " << new_total_complexity.largeTexturesCount
            << " HUD objects cost: " << new_total_complexity.objectsCost
            << " HUD objects count: " << new_total_complexity.objectsCount << LL_ENDL;

        mLatestHUDComplexity = new_total_complexity;
    }
}

bool LLHUDRenderNotifier::isNotificationVisible()
{
    return mHUDNotificationPtr != NULL && mHUDNotificationPtr->isActive();
}

// private static
LLHUDRenderNotifier::EWarnLevel LLHUDRenderNotifier::getWarningType(LLHUDComplexity object_complexity, LLHUDComplexity cmp_complexity)
{
    static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0U); // ties max HUD cost to avatar cost
    static LLCachedControl<U32> max_objects_count(gSavedSettings, "RenderHUDObjectsWarning", WARN_HUD_OBJECTS_LIMIT);
    static LLCachedControl<U32> max_textures_count(gSavedSettings, "RenderHUDTexturesWarning", WARN_HUD_TEXTURES_LIMIT);
    static LLCachedControl<U32> max_oversized_count(gSavedSettings, "RenderHUDOversizedTexturesWarning", WARN_HUD_OVERSIZED_TEXTURES_LIMIT);
    static LLCachedControl<U32> max_texture_memory(gSavedSettings, "RenderHUDTexturesMemoryWarning", WARN_HUD_TEXTURE_MEMORY_LIMIT);

    if (cmp_complexity.texturesMemoryTotal < object_complexity.texturesMemoryTotal
        && object_complexity.texturesMemoryTotal > (F64Bytes)max_texture_memory)
    {
        // Note: Memory might not be accurate since texture is still loading or discard level changes

        LL_DEBUGS("HUDdetail") << "HUD " << object_complexity.objectName << " memory usage over limit, "
            << " was " << cmp_complexity.texturesMemoryTotal
            << " is " << object_complexity.texturesMemoryTotal << LL_ENDL;

        return WARN_MEMORY;
    }
    else if ((cmp_complexity.objectsCost < object_complexity.objectsCost
        || cmp_complexity.texturesCost < object_complexity.texturesCost)
        && max_render_cost > 0
        && object_complexity.objectsCost + object_complexity.texturesCost > max_render_cost)
    {
        LL_DEBUGS("HUDdetail") << "HUD " << object_complexity.objectName << " complexity over limit,"
            << " HUD textures cost: " << object_complexity.texturesCost
            << " HUD objects cost: " << object_complexity.objectsCost << LL_ENDL;

        return WARN_COST;
    }
    else if (cmp_complexity.largeTexturesCount < object_complexity.largeTexturesCount
        && object_complexity.largeTexturesCount > max_oversized_count)
    {
        LL_DEBUGS("HUDdetail") << "HUD " << object_complexity.objectName << " contains to many large textures: "
            << object_complexity.largeTexturesCount << LL_ENDL;

        return WARN_HEAVY;
    }
    else if (cmp_complexity.texturesCount < object_complexity.texturesCount
        && object_complexity.texturesCount > max_textures_count)
    {
        LL_DEBUGS("HUDdetail") << "HUD " << object_complexity.objectName << " contains too many textures: "
            << object_complexity.texturesCount << LL_ENDL;

        return WARN_CRAMPED;
    }
    else if (cmp_complexity.objectsCount < object_complexity.objectsCount
        && object_complexity.objectsCount > max_objects_count)
    {
        LL_DEBUGS("HUDdetail") << "HUD " << object_complexity.objectName << " contains too many objects: "
            << object_complexity.objectsCount << LL_ENDL;

        return WARN_TEXTURES;
    }
    return WARN_NONE;
}

void LLHUDRenderNotifier::displayHUDNotification(EWarnLevel warn_type, LLUUID obj_id, std::string obj_name, std::string joint_name)
{
    static LLCachedControl<U32> pop_up_delay(gSavedSettings, "ComplexityChangesPopUpDelay", 300);
    static LLCachedControl<U32> expire_delay(gSavedSettings, "ShowMyComplexityChanges", 20);
    LLDate expire_date(LLDate::now().secondsSinceEpoch() + expire_delay);

    // Since we need working "ignoretext" there is no other way but to
    // use single notification while constructing it from multiple pieces
    LLSD reason_args;
    if (obj_id.isNull())
    {
        reason_args["HUD_DETAILS"] = LLTrans::getString("hud_description_total");
    }
    else
    {
        if (obj_name.empty())
        {
            LL_WARNS("HUDdetail") << "Object name not assigned" << LL_ENDL;
        }
        if (joint_name.empty())
        {
            std::string verb = "select?name=" + LLURI::escape(obj_name);
            reason_args["HUD_DETAILS"] = LLSLURL("inventory", obj_id, verb.c_str()).getSLURLString();
        }
        else
        {
            LLSD object_args;
            std::string verb = "select?name=" + LLURI::escape(obj_name);
            object_args["OBJ_NAME"] = LLSLURL("inventory", obj_id, verb.c_str()).getSLURLString();
            object_args["JNT_NAME"] = LLTrans::getString(joint_name);
            reason_args["HUD_DETAILS"] = LLTrans::getString("hud_name_with_joint", object_args);
        }
    }

    LLSD msg_args;
    msg_args["HUD_REASON"] = LLTrans::getString(e_hud_messages[warn_type], reason_args);

    mHUDNotificationPtr = LLNotifications::instance().add(LLNotification::Params()
        .name("HUDComplexityWarning")
        .expiry(expire_date)
        .substitutions(msg_args));
    mHUDPopUpDelayTimer.resetWithExpiry(pop_up_delay);
}

