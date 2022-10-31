/** 
* @file   llfloatergroupbulkban.h
* @brief  This floater is a wrapper for LLPanelGroupBulkBan, which
* is used to ban Residents from a specific group.
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

#ifndef LL_LLFLOATERGROUPBULKBAN_H
#define LL_LLFLOATERGROUPBULKBAN_H

#include "llfloater.h"
#include "lluuid.h"

class LLFloaterGroupBulkBan : public LLFloater
{
public:
    virtual ~LLFloaterGroupBulkBan();

    static void showForGroup(const LLUUID& group_id, uuid_vec_t* agent_ids = NULL);

protected:
    LLFloaterGroupBulkBan(const LLUUID& group_id = LLUUID::null);

    class impl;
    impl* mImpl;
};

#endif // LL_LLFLOATERGROUPBULKBAN_H
