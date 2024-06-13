/**
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llpanelavatar.h"

#include "llagent.h"
#include "llloadingindicator.h"
#include "lltooldraganddrop.h"

//////////////////////////////////////////////////////////////////////////
// LLProfileDropTarget

LLProfileDropTarget::LLProfileDropTarget(const LLProfileDropTarget::Params& p)
:   LLView(p),
    mAgentID(p.agent_id)
{}

bool LLProfileDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                     EDragAndDropType cargo_type,
                                     void* cargo_data,
                                     EAcceptance* accept,
                                     std::string& tooltip_msg)
{
    if (getParent())
    {
        LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop,
                                                 cargo_type, cargo_data, accept);

        return true;
    }

    return false;
}

static LLDefaultChildRegistry::Register<LLProfileDropTarget> r("profile_drop_target");

//////////////////////////////////////////////////////////////////////////
// LLPanelProfileTab

LLPanelProfileTab::LLPanelProfileTab()
: LLPanel()
, mAvatarId(LLUUID::null)
, mLoadingState(PROFILE_INIT)
, mSelfProfile(false)
{
}

LLPanelProfileTab::~LLPanelProfileTab()
{
}

void LLPanelProfileTab::setAvatarId(const LLUUID& avatar_id)
{
    if (avatar_id.notNull())
    {
        mAvatarId = avatar_id;
        mSelfProfile = (getAvatarId() == gAgentID);
    }
}

void LLPanelProfileTab::onOpen(const LLSD& key)
{
    // Update data even if we are viewing same avatar profile as some data might been changed.
    setAvatarId(key.asUUID());

    setApplyProgress(true);
}

void LLPanelProfileTab::setLoaded()
{
    setApplyProgress(false);

    mLoadingState = PROFILE_LOADED;
}

void LLPanelProfileTab::setApplyProgress(bool started)
{
    LLLoadingIndicator* indicator = findChild<LLLoadingIndicator>("progress_indicator");

    if (indicator)
    {
        indicator->setVisible(started);

        if (started)
        {
            indicator->start();
        }
        else
        {
            indicator->stop();
        }
    }

    LLView* panel = findChild<LLView>("indicator_stack");
    if (panel)
    {
        panel->setVisible(started);
    }
}

static void put_avatar_properties_coro(std::string cap_url, LLUUID agent_id, LLSD data, std::function<void(bool)> callback)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("put_avatar_properties_coro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    httpOpts->setFollowRedirects(true);

    std::string finalUrl = cap_url + "/" + agent_id.asString();

    LLSD result = httpAdapter->putAndSuspend(httpRequest, finalUrl, data, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("AvatarProperties") << "Failed to put agent information " << data << " for id " << agent_id << LL_ENDL;
    }
    else
    {
        LL_DEBUGS("AvatarProperties") << "Agent id: " << agent_id << " Data: " << data << " Result: " << httpResults << LL_ENDL;
    }

    if (callback)
    {
        callback(status);
    }
}

bool LLPanelProfileTab::saveAgentUserInfoCoro(std::string name, LLSD value, std::function<void(bool)> callback) const
{
    std::string cap_url = gAgent.getRegionCapability("AgentProfile");
    if (cap_url.empty())
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
        return false;
    }

    LLCoros::instance().launch("putAgentUserInfoCoro",
        boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), LLSD().with(name, value), callback));

    return true;
}

LLPanelProfilePropertiesProcessorTab::LLPanelProfilePropertiesProcessorTab()
    : LLPanelProfileTab()
{
}

LLPanelProfilePropertiesProcessorTab::~LLPanelProfilePropertiesProcessorTab()
{
    if (getAvatarId().notNull())
    {
        LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
    }
}

void LLPanelProfilePropertiesProcessorTab::setAvatarId(const LLUUID & avatar_id)
{
    if (avatar_id.notNull())
    {
        if (getAvatarId().notNull())
        {
            LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(), this);
        }
        LLPanelProfileTab::setAvatarId(avatar_id);
        LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(), this);
    }
}

void LLPanelProfilePropertiesProcessorTab::updateData()
{
    LLUUID avatar_id = getAvatarId();
    if (!getStarted() && avatar_id.notNull())
    {
        setIsLoading();
        LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest(getAvatarId());
    }
}
