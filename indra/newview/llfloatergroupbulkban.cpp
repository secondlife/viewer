/** 
* @file llfloatergroupbulkban.cpp
* @brief Floater to ban Residents from a group.
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

#include "llfloatergroupbulkban.h"
#include "llpanelgroupbulkban.h"
#include "lltrans.h"
#include "lldraghandle.h"


class LLFloaterGroupBulkBan::impl
{
public:
    impl(const LLUUID& group_id) : mGroupID(group_id), mBulkBanPanelp(NULL) {}
    ~impl() {}

    static void closeFloater(void* data);

public:
    LLUUID mGroupID;
    LLPanelGroupBulkBan* mBulkBanPanelp;

    static std::map<LLUUID, LLFloaterGroupBulkBan*> sInstances;
};

//
// Globals
//
std::map<LLUUID, LLFloaterGroupBulkBan*> LLFloaterGroupBulkBan::impl::sInstances;

void LLFloaterGroupBulkBan::impl::closeFloater(void* data)
{
    LLFloaterGroupBulkBan* floaterp = (LLFloaterGroupBulkBan*)data;
    if(floaterp)
        floaterp->closeFloater();
}

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------
LLFloaterGroupBulkBan::LLFloaterGroupBulkBan(const LLUUID& group_id/*=LLUUID::null*/)
    : LLFloater(group_id)
{
    S32 floater_header_size = getHeaderHeight();
    LLRect contents;

    mImpl = new impl(group_id);
    mImpl->mBulkBanPanelp = new LLPanelGroupBulkBan(group_id);

    contents = mImpl->mBulkBanPanelp->getRect();
    contents.mTop -= floater_header_size;

    setTitle(mImpl->mBulkBanPanelp->getString("GroupBulkBan"));
    mImpl->mBulkBanPanelp->setCloseCallback(impl::closeFloater, this);
    mImpl->mBulkBanPanelp->setRect(contents);

    addChild(mImpl->mBulkBanPanelp);
}

LLFloaterGroupBulkBan::~LLFloaterGroupBulkBan()
{
    if(mImpl->mGroupID.notNull())
    {
        impl::sInstances.erase(mImpl->mGroupID);
    }

    delete mImpl->mBulkBanPanelp;
    delete mImpl;
}

void LLFloaterGroupBulkBan::showForGroup(const LLUUID& group_id, uuid_vec_t* agent_ids)
{
    const LLFloater::Params& floater_params = LLFloater::getDefaultParams();
    S32 floater_header_size = floater_params.header_height;
    LLRect contents;

    // Make sure group_id isn't null
    if (group_id.isNull())
    {
        LL_WARNS() << "LLFloaterGroupInvite::showForGroup with null group_id!" << LL_ENDL;
        return;
    }

    // If we don't have a floater for this group, create one.
    LLFloaterGroupBulkBan* fgb = get_if_there(impl::sInstances,
        group_id,
        (LLFloaterGroupBulkBan*)NULL);
    if (!fgb)
    {
        fgb = new LLFloaterGroupBulkBan(group_id);
        contents = fgb->mImpl->mBulkBanPanelp->getRect();
        contents.mTop += floater_header_size;
        fgb->setRect(contents);
        fgb->getDragHandle()->setRect(contents);
        fgb->getDragHandle()->setTitle(fgb->mImpl->mBulkBanPanelp->getString("GroupBulkBan"));

        impl::sInstances[group_id] = fgb;

        fgb->mImpl->mBulkBanPanelp->clear();
    }

    if (agent_ids != NULL)
    {
        fgb->mImpl->mBulkBanPanelp->addUsers(*agent_ids);
    }

    fgb->center();
    fgb->openFloater();
    fgb->mImpl->mBulkBanPanelp->update();
}
