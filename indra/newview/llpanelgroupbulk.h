/** 
* @file   llpanelgroupbulk.h
* @brief  Header file for llpanelgroupbulk
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
#ifndef LL_LLPANELGROUPBULK_H
#define LL_LLPANELGROUPBULK_H

#include "llpanel.h"
#include "lluuid.h"

class LLAvatarName;
class LLGroupMgrGroupData;
class LLPanelGroupBulkImpl;

// Base panel class for bulk group invite / ban floaters
class LLPanelGroupBulk : public LLPanel
{
public:
    LLPanelGroupBulk(const LLUUID& group_id);
    /*virtual*/ ~LLPanelGroupBulk();

public: 
    static void callbackClickSubmit(void* userdata) {}
    virtual void submit() = 0;

public:
    virtual void clear();
    virtual void update();
    virtual void draw();

protected:
    virtual void updateGroupName();
    virtual void updateGroupData();

public:
    // this callback is being used to add a user whose fullname isn't been loaded before invoking of addUsers().
    virtual void addUserCallback(const LLUUID& id, const LLAvatarName& av_name);
    virtual void setCloseCallback(void (*close_callback)(void*), void* data);

    virtual void addUsers(uuid_vec_t& agent_ids);

public:
    LLPanelGroupBulkImpl* mImplementation;

protected:
    bool mPendingGroupPropertiesUpdate;
    bool mPendingRoleDataUpdate;
    bool mPendingMemberDataUpdate;
};

#endif // LL_LLPANELGROUPBULK_H

