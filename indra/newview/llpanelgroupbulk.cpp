/**
* @file llpanelgroupbulk.cpp
* @brief Implementation of llpanelgroupbulk
* @author Baker@lindenlab.com
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

#include "llviewerprecompiledheaders.h"

#include "llpanelgroupbulk.h"
#include "llpanelgroupbulkimpl.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llcombobox.h"
#include "llgroupactions.h"
#include "llgroupmgr.h"
#include "llnamelistctrl.h"
#include "llnotificationsutil.h"
#include "llscrolllistitem.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"


//////////////////////////////////////////////////////////////////////////
// Implementation of llpanelgroupbulkimpl.h functions
//////////////////////////////////////////////////////////////////////////
LLPanelGroupBulkImpl::LLPanelGroupBulkImpl(const LLUUID& group_id) :
    mGroupID(group_id),
    mBulkAgentList(nullptr),
    mOKButton(nullptr),
    mAddButton(nullptr),
    mRemoveButton(nullptr),
    mGroupName(nullptr),
    mLoadingText(),
    mTooManySelected(),
    mCloseCallback(nullptr),
    mCloseCallbackUserData(nullptr),
    mRoleNames(nullptr),
    mOwnerWarning(),
    mAlreadyInGroup(),
    mConfirmedOwnerInvite(false),
    mListFullNotificationSent(false)
{
}

LLPanelGroupBulkImpl::~LLPanelGroupBulkImpl()
{
    for (auto& [id, connection] : mAvatarNameCacheConnections)
    {
        if (connection.connected())
            connection.disconnect();
    }

    mAvatarNameCacheConnections.clear();
}

void LLPanelGroupBulkImpl::callbackClickAdd(LLPanelGroupBulk* panelp)
{
    LLFloater* root_floater = gFloaterView->getParentFloater(panelp);
    LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(
        [this](const uuid_vec_t& agent_ids, const std::vector<LLAvatarName>&)
        {
            addUsers(agent_ids);
        }, true, false, false, root_floater->getName(), mAddButton);
    if (picker)
    {
        root_floater->addDependentFloater(picker);
        LLGroupMgr::getInstance()->sendCapGroupMembersRequest(mGroupID);
    }
}

// static
void LLPanelGroupBulkImpl::callbackClickRemove(void* userdata)
{
    if (LLPanelGroupBulkImpl* selfp = (LLPanelGroupBulkImpl*)userdata)
    {
        selfp->handleRemove();
    }
}

// static
void LLPanelGroupBulkImpl::callbackClickCancel(void* userdata)
{
    if (LLPanelGroupBulkImpl* selfp = (LLPanelGroupBulkImpl*)userdata)
    {
        (*(selfp->mCloseCallback))(selfp->mCloseCallbackUserData);
    }
}

// static
void LLPanelGroupBulkImpl::callbackSelect(LLUICtrl* ctrl, void* userdata)
{
    if (LLPanelGroupBulkImpl* selfp = (LLPanelGroupBulkImpl*)userdata)
    {
        selfp->handleSelection();
    }
}

void LLPanelGroupBulkImpl::addUsers(const uuid_vec_t& agent_ids)
{
    for (const LLUUID& agent_id : agent_ids)
    {
        if (LLAvatarName av_name; LLAvatarNameCache::get(agent_id, &av_name))
        {
            onAvatarNameCache(agent_id, av_name);
        }
        else
        {
            if (auto found = mAvatarNameCacheConnections.find(agent_id); found != mAvatarNameCacheConnections.end())
            {
                if (found->second.connected())
                    found->second.disconnect();

                mAvatarNameCacheConnections.erase(found);
            }

            mAvatarNameCacheConnections.try_emplace(agent_id, LLAvatarNameCache::get(agent_id,
                [&](const LLUUID& agent_id, const LLAvatarName& av_name)
                {
                    onAvatarNameCache(agent_id, av_name);
                }));
        }
    }
}

void LLPanelGroupBulkImpl::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    if (auto found = mAvatarNameCacheConnections.find(agent_id); found != mAvatarNameCacheConnections.end())
    {
        if (found->second.connected())
            found->second.disconnect();

        mAvatarNameCacheConnections.erase(found);
    }

    addUsers({ av_name.getCompleteName() }, { agent_id });
}

void LLPanelGroupBulkImpl::handleRemove()
{
    std::vector<LLScrollListItem*> selection = mBulkAgentList->getAllSelected();
    if (selection.empty())
        return;

    for (const LLScrollListItem* item : selection)
    {
        mInviteeIDs.erase(item->getUUID());
    }

    mBulkAgentList->deleteSelectedItems();
    mRemoveButton->setEnabled(false);

    if (mOKButton && mOKButton->getEnabled() && mBulkAgentList->isEmpty())
    {
        mOKButton->setEnabled(false);
    }
}

void LLPanelGroupBulkImpl::handleSelection()
{
    mRemoveButton->setEnabled(mBulkAgentList->getFirstSelected());
}

void LLPanelGroupBulkImpl::addUsers(const std::vector<std::string>& names, const uuid_vec_t& agent_ids)
{
    if (mListFullNotificationSent)
    {
        return;
    }

    if (!mListFullNotificationSent &&
        (names.size() + mInviteeIDs.size() > MAX_GROUP_INVITES))
    {
        mListFullNotificationSent = true;

        // Fail! Show a warning and don't add any names.
        LLSD msg;
        msg["MESSAGE"] = mTooManySelected;
        LLNotificationsUtil::add("GenericAlert", msg);
        return;
    }

    for (S32 i = 0; i < (S32)names.size(); ++i)
    {
        const LLUUID& id = agent_ids[i];

        if (mInviteeIDs.find(id) != mInviteeIDs.end())
        {
            continue;
        }

        // Add the name to the name list
        LLSD row;
        row["id"] = id;
        row["columns"][0]["value"] = names[i];

        mBulkAgentList->addElement(row);
        mInviteeIDs.insert(id);

        // We've successfully added someone to the list.
        if (mOKButton && !mOKButton->getEnabled())
        {
            mOKButton->setEnabled(true);
        }
    }
}

void LLPanelGroupBulkImpl::setGroupName(const std::string& name)
{
    if (mGroupName)
    {
        mGroupName->setText(name);
    }
}


LLPanelGroupBulk::LLPanelGroupBulk(const LLUUID& group_id) :
    LLPanel(),
    mImplementation(new LLPanelGroupBulkImpl(group_id)),
    mPendingGroupPropertiesUpdate(false),
    mPendingRoleDataUpdate(false),
    mPendingMemberDataUpdate(false)
{
}

LLPanelGroupBulk::~LLPanelGroupBulk()
{
    delete mImplementation;
}

void LLPanelGroupBulk::clear()
{
    mImplementation->mInviteeIDs.clear();

    if (mImplementation->mBulkAgentList)
    {
        mImplementation->mBulkAgentList->deleteAllItems();
    }

    if (mImplementation->mOKButton)
    {
        mImplementation->mOKButton->setEnabled(false);
    }
}

void LLPanelGroupBulk::update()
{
    updateGroupName();
    updateGroupData();
}

void LLPanelGroupBulk::draw()
{
    LLPanel::draw();
    update();
}

void LLPanelGroupBulk::updateGroupName()
{
    LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mImplementation->mGroupID);

    if (gdatap &&
        gdatap->isGroupPropertiesDataComplete())
    {
        // Only do work if the current group name differs
        if (mImplementation->mGroupName->getText().compare(gdatap->mName) != 0)
        {
            mImplementation->setGroupName(gdatap->mName);
        }
    }
    else
    {
        mImplementation->setGroupName(mImplementation->mLoadingText);
    }
}

void LLPanelGroupBulk::updateGroupData()
{
    LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mImplementation->mGroupID);
    if (gdatap && gdatap->isGroupPropertiesDataComplete())
    {
        mPendingGroupPropertiesUpdate = false;
    }
    else if (!mPendingGroupPropertiesUpdate)
    {
        mPendingGroupPropertiesUpdate = true;
        LLGroupMgr::getInstance()->sendGroupPropertiesRequest(mImplementation->mGroupID);
    }

    if (gdatap && gdatap->isRoleDataComplete())
    {
        mPendingRoleDataUpdate = false;
    }
    else if (!mPendingRoleDataUpdate)
    {
        mPendingRoleDataUpdate = true;
        LLGroupMgr::getInstance()->sendGroupRoleDataRequest(mImplementation->mGroupID);
    }

    if (gdatap && gdatap->isMemberDataComplete())
    {
        mPendingMemberDataUpdate = false;
    }
    else if (!mPendingMemberDataUpdate)
    {
        mPendingMemberDataUpdate = true;
        LLGroupMgr::getInstance()->sendCapGroupMembersRequest(mImplementation->mGroupID);
    }
}

void LLPanelGroupBulk::addUserCallback(const LLUUID& id, const LLAvatarName& av_name)
{
    mImplementation->addUsers({ av_name.getAccountName() }, { id });
}

void LLPanelGroupBulk::setCloseCallback(void (*close_callback)(void*), void* data)
{
    mImplementation->mCloseCallback         = close_callback;
    mImplementation->mCloseCallbackUserData = data;
}

void LLPanelGroupBulk::addUsers(uuid_vec_t& agent_ids)
{
    std::vector<std::string> names;
    for (size_t i = 0; i < agent_ids.size(); i++)
    {
        std::string fullname;
        const LLUUID& agent_id = agent_ids[i];
        LLViewerObject* dest = gObjectList.findObject(agent_id);
        if (dest && dest->isAvatar())
        {
            LLNameValue* nvfirst = dest->getNVPair("FirstName");
            LLNameValue* nvlast = dest->getNVPair("LastName");
            if (nvfirst && nvlast)
            {
                fullname = LLCacheName::buildFullName(nvfirst->getString(), nvlast->getString());
            }

            if (!fullname.empty())
            {
                names.push_back(fullname);
            }
            else
            {
                LL_WARNS() << "llPanelGroupBulk: Selected avatar has no name: " << dest->getID() << LL_ENDL;
                names.push_back("(Unknown)");
            }
        }
        else
        {
            // Looks like the user tries to invite a friend which is offline.
            // For offline avatar_id gObjectList.findObject() will return null
            // so we need to do this additional search in avatar tracker, see EXT-4732
            if (LLAvatarTracker::instance().isBuddy(agent_id))
            {
                LLAvatarName av_name;
                if (!LLAvatarNameCache::get(agent_id, &av_name))
                {
                    // Actually it shouldn't happen, just in case
                    LLAvatarNameCache::get(LLUUID(agent_id),
                        [&](const LLUUID& agent_id, const LLAvatarName& av_name)
                        {
                            addUserCallback(agent_id, av_name);
                        });
                    // for this special case!
                    // when there is no cached name we should remove resident from agent_ids list to avoid breaking of sequence
                    // removed id will be added in callback
                    agent_ids.erase(agent_ids.begin() + i);
                    i--; // To process the next agent_id with the same index
                }
                else
                {
                    names.push_back(av_name.getAccountName());
                }
            }
        }
    }
    mImplementation->mListFullNotificationSent = false;
    mImplementation->addUsers(names, agent_ids);
}

