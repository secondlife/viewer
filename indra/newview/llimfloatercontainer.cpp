/** 
 * @file llimfloatercontainer.cpp
 * @brief Multifloater containing active IM sessions in separate tab container tabs
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llimfloatercontainer.h"
#include "llfloaterreg.h"
#include "llimview.h"
#include "llavatariconctrl.h"
#include "llagent.h"

//
// LLIMFloaterContainer
//
LLIMFloaterContainer::LLIMFloaterContainer(const LLSD& seed)
:	LLMultiFloater(seed)
{
	mAutoResize = FALSE;
}

LLIMFloaterContainer::~LLIMFloaterContainer()
{
	LLGroupMgr::getInstance()->removeObserver(this);
}

BOOL LLIMFloaterContainer::postBuild()
{
	// Do not call base postBuild to not connect to mCloseSignal to not close all floaters via Close button
	// mTabContainer will be initialized in LLMultiFloater::addChild()
	return TRUE;
}

void LLIMFloaterContainer::onOpen(const LLSD& key)
{
	LLMultiFloater::onOpen(key);
/*
	if (key.isDefined())
	{
		LLIMFloater* im_floater = LLIMFloater::findInstance(key.asUUID());
		if (im_floater)
		{
			im_floater->openFloater();
		}
	}
*/
}

void LLIMFloaterContainer::addFloater(LLFloater* floaterp, 
									BOOL select_added_floater, 
									LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;

	// already here
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}

	LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);

	LLUUID session_id = floaterp->getKey();

	if(gAgent.isInGroup(session_id))
	{
		mSessions[session_id] = floaterp;
		mID = session_id;
		mGroupID.push_back(session_id);
		LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(session_id);
		LLGroupMgr* gm = LLGroupMgr::getInstance();
		gm->addObserver(this);

		if (group_data && group_data->mInsigniaID.notNull())
		{
			mTabContainer->setTabImage(get_ptr_in_map(mSessions, session_id), group_data->mInsigniaID);
		}
		else
		{
			gm->sendGroupPropertiesRequest(session_id);
		}
	}
	else
	{
		LLUUID avatar_id = LLIMModel::getInstance()->getOtherParticipantID(session_id);
		LLAvatarPropertiesProcessor& app = LLAvatarPropertiesProcessor::instance();
		app.addObserver(avatar_id, this);
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, avatar_id));
		mSessions[avatar_id] = floaterp;

		LLUUID* icon_id_ptr = LLAvatarIconIDCache::getInstance()->get(avatar_id);
		if(!icon_id_ptr)
		{
			app.sendAvatarPropertiesRequest(avatar_id);
		}
		else
		{
			mTabContainer->setTabImage(floaterp, *icon_id_ptr);
		}
	}
}

void LLIMFloaterContainer::processProperties(void* data, enum EAvatarProcessorType type)
{
	if (APT_PROPERTIES == type)
	{
			LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);
			if (avatar_data)
			{
				LLUUID avatar_id = avatar_data->avatar_id;
				if(avatar_data->image_id != *LLAvatarIconIDCache::getInstance()->get(avatar_id))
				{
					LLAvatarIconIDCache::getInstance()->add(avatar_id,avatar_data->image_id);
				}
				mTabContainer->setTabImage(get_ptr_in_map(mSessions, avatar_id), avatar_data->image_id);
			}
	}
}

void LLIMFloaterContainer::changed(LLGroupChange gc)
{
	if (GC_PROPERTIES == gc)
	{
		for(groupIDs_t::iterator it = mGroupID.begin(); it!=mGroupID.end(); it++)
		{
			LLUUID group_id = *it;
			LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(group_id);
			if (group_data && group_data->mInsigniaID.notNull())
			{
				mTabContainer->setTabImage(get_ptr_in_map(mSessions, group_id), group_data->mInsigniaID);
			}
		}
	}
}

void LLIMFloaterContainer::onCloseFloater(LLUUID id)
{
	LLAvatarPropertiesProcessor::instance().removeObserver(id, this);
}

LLIMFloaterContainer* LLIMFloaterContainer::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLIMFloaterContainer>("im_container");
}

LLIMFloaterContainer* LLIMFloaterContainer::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLIMFloaterContainer>("im_container");
}

// EOF
