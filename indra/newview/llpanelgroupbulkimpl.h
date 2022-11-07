/** 
* @file   llpanelgroupbulkimpl.h
* @brief  Class definition for implementation class of LLPanelGroupBulk
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
#ifndef LL_LLPANELGROUPBULKIMPL_H
#define LL_LLPANELGROUPBULKIMPL_H

#include "llpanel.h"
#include "lluuid.h"

class LLAvatarName;
class LLNameListCtrl;
class LLTextBox;
class LLComboBox;

//////////////////////////////////////////////////////////////////////////
//  Implementation found in llpanelgroupbulk.cpp
//////////////////////////////////////////////////////////////////////////
class LLPanelGroupBulkImpl
{
public:
    LLPanelGroupBulkImpl(const LLUUID& group_id);
    ~LLPanelGroupBulkImpl();

    static void callbackClickAdd(void* userdata);
    static void callbackClickRemove(void* userdata);

    static void callbackClickCancel(void* userdata);

    static void callbackSelect(LLUICtrl* ctrl, void* userdata);
    static void callbackAddUsers(const uuid_vec_t& agent_ids, void* user_data);

    static void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name, void* user_data);

    void handleRemove();
    void handleSelection();

    void addUsers(const std::vector<std::string>& names, const uuid_vec_t& agent_ids);
    void setGroupName(std::string name);


public:
    static const S32 MAX_GROUP_INVITES = 100;   // Max invites per request. 100 to match server cap.
    

    LLUUID              mGroupID;

    LLNameListCtrl*     mBulkAgentList;
    LLButton*           mOKButton;
    LLButton*           mRemoveButton;
    LLTextBox*          mGroupName;

    std::string         mLoadingText; 
    std::string         mTooManySelected;
    std::string         mBanNotPermitted;
    std::string         mBanLimitFail;
    std::string         mCannotBanYourself;

    std::set<LLUUID>    mInviteeIDs;

    void (*mCloseCallback)(void* data);
    void* mCloseCallbackUserData;
    boost::signals2::connection mAvatarNameCacheConnection;

    // The following are for the LLPanelGroupInvite subclass only.  
    // These aren't needed for LLPanelGroupBulkBan, but if we have to add another 
    // group bulk floater for some reason, we'll have these objects too.
public:
    LLComboBox*     mRoleNames;
    std::string     mOwnerWarning;
    std::string     mAlreadyInGroup;
    bool            mConfirmedOwnerInvite;
    bool            mListFullNotificationSent;
};

#endif // LL_LLPANELGROUPBULKIMPL_H

