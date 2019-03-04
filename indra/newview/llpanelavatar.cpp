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

void LLProfileDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
    LL_INFOS() << "LLProfileDropTarget::doDrop()" << LL_ENDL;
}

BOOL LLProfileDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
                                     EDragAndDropType cargo_type,
                                     void* cargo_data,
                                     EAcceptance* accept,
                                     std::string& tooltip_msg)
{
    if (getParent())
    {
        LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop,
                                                 cargo_type, cargo_data, accept);

        return TRUE;
    }

    return FALSE;
}

static LLDefaultChildRegistry::Register<LLProfileDropTarget> r("profile_drop_target");

//////////////////////////////////////////////////////////////////////////
// LLPanelProfileTab

LLPanelProfileTab::LLPanelProfileTab()
: LLPanel()
, mAvatarId(LLUUID::null)
, mLoading(false)
, mLoaded(false)
, mEmbedded(false)
, mSelfProfile(false)
{
}

LLPanelProfileTab::~LLPanelProfileTab()
{
    if(getAvatarId().notNull())
    {
        LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
    }
}

void LLPanelProfileTab::setAvatarId(const LLUUID& avatar_id)
{
    if (avatar_id.notNull())
    {
        if (getAvatarId().notNull())
        {
            LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId, this);
        }
        mAvatarId = avatar_id;
        LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(), this);

        mSelfProfile = (getAvatarId() == gAgentID);
    }
}

void LLPanelProfileTab::onOpen(const LLSD& key)
{
    // Update data even if we are viewing same avatar profile as some data might been changed.
    setAvatarId(key.asUUID());

    setApplyProgress(true);
}

void LLPanelProfileTab::updateButtons()
{
    setApplyProgress(false);

    mLoaded = true;
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
}
