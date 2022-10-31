/** 
 * @file llfloatergroupinvite.h
 * @brief This floater is just a wrapper for LLPanelGroupInvite, which
 * is used to invite members to a specific group
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLFLOATERGROUPINVITE_H
#define LL_LLFLOATERGROUPINVITE_H

#include "llfloater.h"
#include "lluuid.h"

class LLFloaterGroupInvite
: public LLFloater
{
public:
    virtual ~LLFloaterGroupInvite();

    static void showForGroup(const LLUUID &group_id, uuid_vec_t *agent_ids = NULL, bool request_update = true);

protected:
    LLFloaterGroupInvite(const LLUUID& group_id = LLUUID::null);

    class impl;
    impl* mImpl;
};

#endif
