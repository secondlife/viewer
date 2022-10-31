/** 
* @file llpanelgroupbulkban.cpp
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

#include "llpanelgroupbulkban.h"
#include "llpanelgroupbulk.h"
#include "llpanelgroupbulkimpl.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llavataractions.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llcombobox.h"
#include "llgroupactions.h"
#include "llgroupmgr.h"
#include "llnamelistctrl.h"
#include "llnotificationsutil.h"
#include "llscrolllistitem.h"
#include "llslurl.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"

#include <boost/foreach.hpp>

LLPanelGroupBulkBan::LLPanelGroupBulkBan(const LLUUID& group_id) : LLPanelGroupBulk(group_id)
{
    // Pass on construction of this panel to the control factory.
    buildFromFile( "panel_group_bulk_ban.xml");
}

BOOL LLPanelGroupBulkBan::postBuild()
{
    BOOL recurse = TRUE;

    mImplementation->mLoadingText = getString("loading");
    mImplementation->mGroupName = getChild<LLTextBox>("group_name_text", recurse);
    mImplementation->mBulkAgentList = getChild<LLNameListCtrl>("banned_agent_list", recurse);
    if ( mImplementation->mBulkAgentList )
    {
        mImplementation->mBulkAgentList->setCommitOnSelectionChange(TRUE);
        mImplementation->mBulkAgentList->setCommitCallback(LLPanelGroupBulkImpl::callbackSelect, mImplementation);
    }

    LLButton* button = getChild<LLButton>("add_button", recurse);
    if ( button )
    {
        // default to opening avatarpicker automatically
        // (*impl::callbackClickAdd)((void*)this);
        button->setClickedCallback(LLPanelGroupBulkImpl::callbackClickAdd, this);
    }

    mImplementation->mRemoveButton = 
        getChild<LLButton>("remove_button", recurse);
    if ( mImplementation->mRemoveButton )
    {
        mImplementation->mRemoveButton->setClickedCallback(LLPanelGroupBulkImpl::callbackClickRemove, mImplementation);
        mImplementation->mRemoveButton->setEnabled(FALSE);
    }

    mImplementation->mOKButton = 
        getChild<LLButton>("ban_button", recurse);
    if ( mImplementation->mOKButton )
    {
        mImplementation->mOKButton->setClickedCallback(LLPanelGroupBulkBan::callbackClickSubmit, this);
        mImplementation->mOKButton->setEnabled(FALSE);
    }

    button = getChild<LLButton>("cancel_button", recurse);
    if ( button )
    {
        button->setClickedCallback(LLPanelGroupBulkImpl::callbackClickCancel, mImplementation);
    }

    mImplementation->mTooManySelected = getString("ban_selection_too_large");
    mImplementation->mBanNotPermitted = getString("ban_not_permitted");
    mImplementation->mBanLimitFail = getString("ban_limit_fail");
    mImplementation->mCannotBanYourself = getString("cant_ban_yourself");

    update();
    return TRUE;
}

// TODO: Refactor the shitty callback functions with void* -- just use boost::bind to call submit() instead.
void LLPanelGroupBulkBan::callbackClickSubmit(void* userdata)
{
    LLPanelGroupBulkBan* selfp = (LLPanelGroupBulkBan*)userdata;

    if(selfp)
        selfp->submit();
}


void LLPanelGroupBulkBan::submit()
{
    if (!gAgent.hasPowerInGroup(mImplementation->mGroupID, GP_GROUP_BAN_ACCESS))
    {
        // Fail! Agent no longer have ban rights. Permissions could have changed after button was pressed.
        LLSD msg;
        msg["MESSAGE"] = mImplementation->mBanNotPermitted;
        LLNotificationsUtil::add("GenericAlert", msg);
        (*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
        return;
    }
    LLGroupMgrGroupData * group_datap = LLGroupMgr::getInstance()->getGroupData(mImplementation->mGroupID);
    if (group_datap && group_datap->mBanList.size() >= GB_MAX_BANNED_AGENTS)
    {
        // Fail! Size limit exceeded. List could have updated after button was pressed.
        LLSD msg;
        msg["MESSAGE"] = mImplementation->mBanLimitFail;
        LLNotificationsUtil::add("GenericAlert", msg);
        (*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
        return;
    }
    std::vector<LLUUID> banned_agent_list;  
    std::vector<LLScrollListItem*> agents = mImplementation->mBulkAgentList->getAllData();
    std::vector<LLScrollListItem*>::iterator iter = agents.begin();
    for(;iter != agents.end(); ++iter)
    {
        LLScrollListItem* agent = *iter;
        banned_agent_list.push_back(agent->getUUID());
    }

    const S32 MAX_BANS_PER_REQUEST = 100; // Max bans per request. 100 to match server cap.
    if (banned_agent_list.size() > MAX_BANS_PER_REQUEST)
    {
        // Fail!
        LLSD msg;
        msg["MESSAGE"] = mImplementation->mTooManySelected;
        LLNotificationsUtil::add("GenericAlert", msg);
        (*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
        return;
    }

    // remove already banned users and yourself from request.
    std::vector<LLAvatarName> banned_avatar_names;
    std::vector<LLAvatarName> out_of_limit_names;
    bool banning_self = FALSE;
    std::vector<LLUUID>::iterator conflict = std::find(banned_agent_list.begin(), banned_agent_list.end(), gAgent.getID());
    if (conflict != banned_agent_list.end())
    {
        banned_agent_list.erase(conflict);
        banning_self = TRUE;
    }
    if (group_datap)
    {
        BOOST_FOREACH(const LLGroupMgrGroupData::ban_list_t::value_type& group_ban_pair, group_datap->mBanList)
        {
            const LLUUID& group_ban_agent_id = group_ban_pair.first;
            std::vector<LLUUID>::iterator conflict = std::find(banned_agent_list.begin(), banned_agent_list.end(), group_ban_agent_id);
            if (conflict != banned_agent_list.end())
            {
                LLAvatarName av_name;
                LLAvatarNameCache::get(group_ban_agent_id, &av_name);
                banned_avatar_names.push_back(av_name);

                banned_agent_list.erase(conflict);
                if (banned_agent_list.size() == 0)
                {
                    break;
                }
            }
        }
        // this check should always be the last one before we send the request.
        // Otherwise we have a possibility of cutting more then we need to.
        if (banned_agent_list.size() > GB_MAX_BANNED_AGENTS - group_datap->mBanList.size())
        {
            std::vector<LLUUID>::iterator exeedes_limit = banned_agent_list.begin() + GB_MAX_BANNED_AGENTS - group_datap->mBanList.size();
            for (std::vector<LLUUID>::iterator itor = exeedes_limit ; 
                itor != banned_agent_list.end(); ++itor)
            {
                LLAvatarName av_name;
                LLAvatarNameCache::get(*itor, &av_name);
                out_of_limit_names.push_back(av_name);
            }
            banned_agent_list.erase(exeedes_limit,banned_agent_list.end());
        }
    }

    // sending request and ejecting members
    if (banned_agent_list.size() != 0)
    {
        LLGroupMgr::getInstance()->sendGroupBanRequest(LLGroupMgr::REQUEST_POST, mImplementation->mGroupID, LLGroupMgr::BAN_CREATE | LLGroupMgr::BAN_UPDATE, banned_agent_list);
        LLGroupMgr::getInstance()->sendGroupMemberEjects(mImplementation->mGroupID, banned_agent_list);
    }

    // building notification
    if (banned_avatar_names.size() > 0 || banning_self || out_of_limit_names.size() > 0)
    {
        std::string reasons;
        if(banned_avatar_names.size() > 0)
        {
            reasons = "\n " + buildResidentsArgument(banned_avatar_names, "residents_already_banned");
        }

        if(banning_self)
        {
            reasons += "\n " + mImplementation->mCannotBanYourself;
        }

        if(out_of_limit_names.size() > 0)
        {
            reasons += "\n " + buildResidentsArgument(out_of_limit_names, "ban_limit_reached");
        }

        LLStringUtil::format_map_t msg_args;
        msg_args["[REASONS]"] = reasons;
        LLSD msg;
        if (banned_agent_list.size() == 0)
        {
            msg["MESSAGE"] = getString("ban_failed", msg_args);
        }
        else
        {
            msg["MESSAGE"] = getString("partial_ban", msg_args);
        }
        LLNotificationsUtil::add("GenericAlert", msg);
    }
    
    //then close
    (*(mImplementation->mCloseCallback))(mImplementation->mCloseCallbackUserData);
}

std::string LLPanelGroupBulkBan::buildResidentsArgument(std::vector<LLAvatarName> avatar_names, const std::string &format)
{
    std::string names_string;
    LLAvatarActions::buildResidentsString(avatar_names, names_string);
    LLStringUtil::format_map_t args;
    args["[RESIDENTS]"] = names_string;
    return getString(format, args);
}
