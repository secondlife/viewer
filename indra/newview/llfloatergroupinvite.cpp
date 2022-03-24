/** 
 * @file llfloatergroupinvite.cpp
 * @brief Floater to invite new members into a group.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatergroupinvite.h"
#include "llpanelgroupinvite.h"
#include "lltrans.h"
#include "lldraghandle.h"
#include "llagent.h"
#include "llgroupmgr.h"

class LLFloaterGroupInvite::impl
{
public:
	impl(const LLUUID& group_id);
	~impl();

	static void closeFloater(void* data);

public:
	LLUUID mGroupID;
	LLPanelGroupInvite*	mInvitePanelp;

	static std::map<LLUUID, LLFloaterGroupInvite*> sInstances;
};

//
// Globals
//
std::map<LLUUID, LLFloaterGroupInvite*> LLFloaterGroupInvite::impl::sInstances;

LLFloaterGroupInvite::impl::impl(const LLUUID& group_id) :
	mGroupID(group_id),
	mInvitePanelp(NULL)
{
}

LLFloaterGroupInvite::impl::~impl()
{
}

//static
void LLFloaterGroupInvite::impl::closeFloater(void* data)
{
	LLFloaterGroupInvite* floaterp = (LLFloaterGroupInvite*) data;

	if ( floaterp ) floaterp->closeFloater();
}

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------
LLFloaterGroupInvite::LLFloaterGroupInvite(const LLUUID& group_id)
:	LLFloater(group_id)
{
	S32 floater_header_size = getHeaderHeight();
	LLRect contents;

	mImpl = new impl(group_id);

	mImpl->mInvitePanelp = new LLPanelGroupInvite(group_id);

	contents = mImpl->mInvitePanelp->getRect();
	contents.mTop -= floater_header_size;

	setTitle (mImpl->mInvitePanelp->getString("GroupInvitation"));

	mImpl->mInvitePanelp->setCloseCallback(impl::closeFloater, this);

	mImpl->mInvitePanelp->setRect(contents);
	addChild(mImpl->mInvitePanelp);
}

// virtual
LLFloaterGroupInvite::~LLFloaterGroupInvite()
{
	if (mImpl->mGroupID.notNull())
	{
		impl::sInstances.erase(mImpl->mGroupID);
	}

	delete mImpl->mInvitePanelp;
	delete mImpl;
}

// static
void LLFloaterGroupInvite::showForGroup(const LLUUID& group_id, uuid_vec_t *agent_ids, bool request_update)
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
	LLFloaterGroupInvite *fgi = get_if_there(impl::sInstances,
											 group_id,
											 (LLFloaterGroupInvite*)NULL);

	if (request_update)
	{
		// refresh group information
		gAgent.sendAgentDataUpdateRequest();
		LLGroupMgr::getInstance()->clearGroupData(group_id);
	}


	if (!fgi)
	{
		fgi = new LLFloaterGroupInvite(group_id);
		contents = fgi->mImpl->mInvitePanelp->getRect();
		contents.mTop += floater_header_size;
		fgi->setRect(contents);
		fgi->getDragHandle()->setRect(contents);
		fgi->getDragHandle()->setTitle(fgi->mImpl->mInvitePanelp->getString("GroupInvitation"));

		impl::sInstances[group_id] = fgi;

		fgi->mImpl->mInvitePanelp->clear();
	}

	if (agent_ids != NULL)
	{
		fgi->mImpl->mInvitePanelp->addUsers(*agent_ids);
	}
	
	fgi->center();
	fgi->openFloater();
	fgi->mImpl->mInvitePanelp->update();
}
