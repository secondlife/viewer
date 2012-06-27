/** 
 * @file llimfloatercontainer.cpp
 * @brief Multifloater containing active IM sessions in separate tab container tabs
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llimfloatercontainer.h"
#include "llfloaterreg.h"
#include "llimview.h"
#include "llavatariconctrl.h"
#include "llgroupiconctrl.h"
#include "llagent.h"
#include "lltransientfloatermgr.h"

//
// LLIMFloaterContainer
//
LLIMFloaterContainer::LLIMFloaterContainer(const LLSD& seed)
:	LLMultiFloater(seed)
{
	mAutoResize = FALSE;
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
}

LLIMFloaterContainer::~LLIMFloaterContainer()
{
	mNewMessageConnection.disconnect();
	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
}

BOOL LLIMFloaterContainer::postBuild()
{
	mNewMessageConnection = LLIMModel::instance().mNewMsgSignal.connect(boost::bind(&LLIMFloaterContainer::onNewMessageReceived, this, _1));
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

	if(gAgent.isInGroup(session_id, TRUE))
	{
		LLGroupIconCtrl::Params icon_params;
		icon_params.group_id = session_id;
		icon = LLUICtrlFactory::instance().create<LLGroupIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	else
	{
		LLUUID avatar_id = LLIMModel::getInstance()->getOtherParticipantID(session_id);

		LLAvatarIconCtrl::Params icon_params;
		icon_params.avatar_id = avatar_id;
		icon = LLUICtrlFactory::instance().create<LLAvatarIconCtrl>(icon_params);

		mSessions[session_id] = floaterp;
		floaterp->mCloseSignal.connect(boost::bind(&LLIMFloaterContainer::onCloseFloater, this, session_id));
	}
	mTabContainer->setTabImage(floaterp, icon);
}

void LLIMFloaterContainer::onCloseFloater(LLUUID& id)
{
	mSessions.erase(id);
	setFocus(TRUE);
}

void LLIMFloaterContainer::onNewMessageReceived(const LLSD& data)
{
	LLUUID session_id = data["session_id"].asUUID();
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

void LLIMFloaterContainer::setMinimized(BOOL b)
{
	if (isMinimized() == b) return;
	
	LLMultiFloater::setMinimized(b);
	// Hide minimized floater (see EXT-5315)
	setVisible(!b);

	if (isMinimized()) return;

	if (getActiveFloater())
	{
		getActiveFloater()->setVisible(TRUE);
	}
}

// EOF
