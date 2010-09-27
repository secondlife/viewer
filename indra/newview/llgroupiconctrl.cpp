/** 
 * @file llgroupiconctrl.cpp
 * @brief LLGroupIconCtrl class implementation
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

#include "llgroupiconctrl.h"

#include "llagent.h"
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

