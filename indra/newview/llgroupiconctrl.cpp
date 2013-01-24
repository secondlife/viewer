/** 
 * @file llgroupiconctrl.cpp
 * @brief LLGroupIconCtrl class implementation
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

#include "llgroupiconctrl.h"

#include "llagent.h"
#include "llviewertexture.h"
/*
#include "llavatarconstants.h"
#include "llcallingcard.h" // for LLAvatarTracker
#include "llavataractions.h"
#include "llmenugl.h"
#include "lluictrlfactory.h"

#include "llcachename.h"
#include "llagentdata.h"
#include "llimfloater.h"
*/

static LLDefaultChildRegistry::Register<LLGroupIconCtrl> g_i("group_icon");

LLGroupIconCtrl::Params::Params()
:	group_id("group_id")
,	draw_tooltip("draw_tooltip", true)
,	default_icon_name("default_icon_name")
{
}


LLGroupIconCtrl::LLGroupIconCtrl(const LLGroupIconCtrl::Params& p)
:	LLIconCtrl(p)
,	mGroupId(LLUUID::null)
,	mDrawTooltip(p.draw_tooltip)
,	mDefaultIconName(p.default_icon_name)
{
	mPriority = LLViewerFetchedTexture::BOOST_ICON;

	if (p.group_id.isProvided())
	{
		LLSD value(p.group_id);
		setValue(value);
	}
	else
	{
		LLIconCtrl::setValue(mDefaultIconName);
	}
}

LLGroupIconCtrl::~LLGroupIconCtrl()
{
	LLGroupMgr::getInstance()->removeObserver(this);
}

void LLGroupIconCtrl::setValue(const LLSD& value)
{
	if (value.isUUID())
	{
		LLGroupMgr* gm = LLGroupMgr::getInstance();
		if (mGroupId.notNull())
		{
			gm->removeObserver(this);
		}

		if (mGroupId != value.asUUID())
		{
			mGroupId = value.asUUID();
			mID = mGroupId; // set LLGroupMgrObserver::mID to make callbacks work

			// Check if cache already contains image_id for that group
			if (!updateFromCache())
			{
				LLIconCtrl::setValue(mDefaultIconName);
				gm->addObserver(this);
				gm->sendGroupPropertiesRequest(mGroupId);
			}
		}
	}
	else
	{
		LLIconCtrl::setValue(value);
	}
}

void LLGroupIconCtrl::changed(LLGroupChange gc)
{
	if (GC_PROPERTIES == gc)
	{
		updateFromCache();
	}
}

bool LLGroupIconCtrl::updateFromCache()
{
	LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(mGroupId);
	if (!group_data) return false;

	if (group_data->mInsigniaID.notNull())
	{
		LLIconCtrl::setValue(group_data->mInsigniaID);
	}
	else
	{
		LLIconCtrl::setValue(mDefaultIconName);
	}

	if (mDrawTooltip && !group_data->mName.empty())
	{
		setToolTip(group_data->mName);
	}
	else
	{
		setToolTip(LLStringUtil::null);
	}
	return true;
}

