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
#include "llgroupiconctrl.h"
#include "llagent.h"

//
// LLIMFloaterContainer
//
LLIMFloaterContainer::LLIMFloaterContainer(const LLSD& seed)
:	LLMultiFloater(seed)
{
	mAutoResize = FALSE;
}

LLIMFloaterContainer::~LLIMFloaterContainer(){}

BOOL LLIMFloaterContainer::postBuild()
{
	LLIMModel::instance().mNewMsgSignal.connect(boost::bind(&LLIMFloaterContainer::onNewMessageReceived, this, _1));
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

	LLIconCtrl* icon = 0;

	if(gAgent.isInGroup(session_id))
	{
		LLGroupIconCtrl::Params icon_params = LLUICtrlFactory::instance().getDefaultParams<LLGroupIconCtrl>();
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().createWidget<LLGroupIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	else
	{
		LLUUID avatar_id = LLIMModel::getInstance()->getOtherParticipantID(session_id);

		LLAvatarIconCtrl::Params icon_params = LLUICtrlFactory::instance().getDefaultParams<LLAvatarIconCtrl>();
		icon_params.avatar_id = avatar_id;
		icon = LLUICtrlFactory::instance().createWidget<LLAvatarIconCtrl>(icon_params);

		mSessions[avatar_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, avatar_id));
	}
	mTabContainer->setTabImage(floaterp, icon);
}

void LLIMFloaterContainer::onCloseFloater(LLUUID& id)
{
	mSessions.erase(id);
}

void LLIMFloaterContainer::processProperties(void* data, enum EAvatarProcessorType type)
{
	if (APT_PROPERTIES == type)
	{
		LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);
		if (avatar_data)
		{
			LLUUID avatar_id = avatar_data->avatar_id;
			LLUUID* cached_avatarId = LLAvatarIconIDCache::getInstance()->get(avatar_id);
			if(cached_avatarId && cached_avatarId->notNull() && avatar_data->image_id != *cached_avatarId)
			{
				LLAvatarIconIDCache::getInstance()->add(avatar_id,avatar_data->image_id);
				mTabContainer->setTabImage(get_ptr_in_map(mSessions, avatar_id), avatar_data->image_id);
			}
		}
	}
}

void LLIMFloaterContainer::changed(const LLUUID& group_id, LLGroupChange gc)
{
	if (GC_PROPERTIES == gc)
	{
		LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(group_id);
		if (group_data && group_data->mInsigniaID.notNull())
		{
			mTabContainer->setTabImage(get_ptr_in_map(mSessions, group_id), group_data->mInsigniaID);
		}
	}
}

void LLIMFloaterContainer::onNewMessageReceived(const LLSD& data)
{
	LLUUID session_id = data["from_id"].asUUID();
	LLFloater* floaterp = get_ptr_in_map(mSessions, session_id);
	LLFloater* current_floater = LLMultiFloater::getActiveFloater();

	if(floaterp && current_floater && floaterp != current_floater)
	{
		if(LLMultiFloater::isFloaterFlashing(floaterp))
			LLMultiFloater::setFloaterFlashing(floaterp, FALSE);
		LLMultiFloater::setFloaterFlashing(floaterp, TRUE);
	}
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
