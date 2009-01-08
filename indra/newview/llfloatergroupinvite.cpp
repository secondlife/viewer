/** 
 * @file llfloatergroupinvite.cpp
 * @brief Floater to invite new members into a group.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatergroupinvite.h"
#include "llpanelgroupinvite.h"

const char FLOATER_TITLE[] = "Group Invitation";
const LLRect FGI_RECT(0, 380, 210, 0);

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

	if ( floaterp ) floaterp->close();
}

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------
LLFloaterGroupInvite::LLFloaterGroupInvite(const std::string& name,
										   const LLRect &rect,
										   const std::string& title,
										   const LLUUID& group_id)
:	LLFloater(name, rect, title)
{
	LLRect contents(getRect());
	contents.mTop -= LLFLOATER_HEADER_SIZE;

	mImpl = new impl(group_id);

	mImpl->mInvitePanelp = new LLPanelGroupInvite("Group Invite Panel",
												  group_id);

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
void LLFloaterGroupInvite::showForGroup(const LLUUID& group_id, std::vector<LLUUID> *agent_ids)
{
	// Make sure group_id isn't null
	if (group_id.isNull())
	{
		llwarns << "LLFloaterGroupInvite::showForGroup with null group_id!" << llendl;
		return;
	}

	// If we don't have a floater for this group, create one.
	LLFloaterGroupInvite *fgi = get_if_there(impl::sInstances,
											 group_id,
											 (LLFloaterGroupInvite*)NULL);
	if (!fgi)
	{
		fgi = new LLFloaterGroupInvite("groupinfo",
									   FGI_RECT,
									   FLOATER_TITLE,
									   group_id);

		impl::sInstances[group_id] = fgi;

		fgi->mImpl->mInvitePanelp->clear();
	}

	if (agent_ids != NULL)
	{
		fgi->mImpl->mInvitePanelp->addUsers(*agent_ids);
	}
	
	fgi->center();
	fgi->open();	/*Flawfinder: ignore*/
	fgi->mImpl->mInvitePanelp->update();
}
