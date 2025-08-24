/**
 * @file llfloaterchatmentionpicker.cpp
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#include "llfloaterchatmentionpicker.h"

#include "llavatarlist.h"
#include "llfloaterimcontainer.h"
#include "llchatmentionhelper.h"
#include "llparticipantlist.h"

LLUUID LLFloaterChatMentionPicker::sSessionID(LLUUID::null);

LLFloaterChatMentionPicker::LLFloaterChatMentionPicker(const LLSD& key)
: LLFloater(key), mAvatarList(NULL)
{
    // This floater should hover on top of our dependent (with the dependent having the focus)
    setFocusStealsFrontmost(false);
    setBackgroundVisible(false);
    setAutoFocus(false);
}

bool LLFloaterChatMentionPicker::postBuild()
{
    mAvatarList = getChild<LLAvatarList>("avatar_list");
    mAvatarList->setShowCompleteName(true, true);
    mAvatarList->setFocusOnItemClicked(false);
    mAvatarList->setItemClickedCallback([this](LLUICtrl* ctrl, S32 x, S32 y, MASK mask)
    {
        if (LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(ctrl))
        {
            selectResident(item->getAvatarId());
        }
    });
    mAvatarList->setRefreshCompleteCallback([this](LLUICtrl* ctrl, const LLSD& param)
    {
        if (mAvatarList->numSelected() == 0)
        {
            mAvatarList->selectFirstItem();
        }
    });

    return LLFloater::postBuild();
}

void LLFloaterChatMentionPicker::onOpen(const LLSD& key)
{
    buildAvatarList();
    mAvatarList->setNameFilter(key.has("av_name") ? key["av_name"].asString() : "");

    gFloaterView->adjustToFitScreen(this, false);
}

uuid_vec_t LLFloaterChatMentionPicker::getParticipantIds()
{
    LLParticipantList* item = dynamic_cast<LLParticipantList*>(LLFloaterIMContainer::getInstance()->getSessionModel(sSessionID));
    if (!item)
    {
        LL_WARNS() << "Participant list is missing" << LL_ENDL;
        return {};
    }

    uuid_vec_t avatar_ids;
    LLFolderViewModelItemCommon::child_list_t::const_iterator current_participant_model = item->getChildrenBegin();
    LLFolderViewModelItemCommon::child_list_t::const_iterator end_participant_model = item->getChildrenEnd();
    while (current_participant_model != end_participant_model)
    {
        LLConversationItem* participant_model = dynamic_cast<LLConversationItem*>((*current_participant_model).get());
        if (participant_model)
        {
            avatar_ids.push_back(participant_model->getUUID());
        }
        current_participant_model++;
    }
    return avatar_ids;
}

void LLFloaterChatMentionPicker::buildAvatarList()
{
    uuid_vec_t& avatar_ids = mAvatarList->getIDs();
    avatar_ids = getParticipantIds();
    updateAvatarList(avatar_ids);
    mAvatarList->setDirty();
}

void LLFloaterChatMentionPicker::selectResident(const LLUUID& id)
{
    if (id.isNull())
        return;

    setValue(stringize("secondlife:///app/agent/", id.asString(), "/mention "));
    onCommit();
    LLChatMentionHelper::instance().hideHelper();
}

void LLFloaterChatMentionPicker::onClose(bool app_quitting)
{
    if (!app_quitting)
    {
        LLChatMentionHelper::instance().hideHelper();
    }
}

bool LLFloaterChatMentionPicker::handleKey(KEY key, MASK mask, bool called_from_parent)
{
    if (mask == MASK_NONE)
    {
        switch (key)
        {
            case KEY_UP:
            case KEY_DOWN:
                return mAvatarList->handleKey(key, mask, called_from_parent);
            case KEY_RETURN:
            case KEY_TAB:
                selectResident(mAvatarList->getSelectedUUID());
                return true;
            case KEY_ESCAPE:
                LLChatMentionHelper::instance().hideHelper();
                return true;
            case KEY_LEFT:
            case KEY_RIGHT:
                return true;
            default:
                break;
        }
    }
    return LLFloater::handleKey(key, mask, called_from_parent);
}

void LLFloaterChatMentionPicker::goneFromFront()
{
    LLChatMentionHelper::instance().hideHelper();
}

void LLFloaterChatMentionPicker::updateSessionID(LLUUID session_id)
{
    sSessionID = session_id;

    LLParticipantList* item = dynamic_cast<LLParticipantList*>(LLFloaterIMContainer::getInstance()->getSessionModel(sSessionID));
    if (!item)
    {
        LL_WARNS() << "Participant list is missing" << LL_ENDL;
        return;
    }

    uuid_vec_t avatar_ids = getParticipantIds();
    updateAvatarList(avatar_ids);
}

void LLFloaterChatMentionPicker::updateAvatarList(uuid_vec_t& avatar_ids)
{
    std::vector<std::string> av_names;
    for (auto& id : avatar_ids)
    {
        LLAvatarName av_name;
        LLAvatarNameCache::get(id, &av_name);
        av_names.push_back(utf8str_tolower(av_name.getAccountName()));
        av_names.push_back(utf8str_tolower(av_name.getDisplayName()));
    }
    LLChatMentionHelper::instance().updateAvatarList(av_names);
}
