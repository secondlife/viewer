/** 
 * @file llavatariconctrl.cpp
 * @brief LLAvatarIconCtrl class implementation
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

#include "llavatariconctrl.h"

#include "llagent.h"
#include "llavatarconstants.h"
#include "llcallingcard.h" // for LLAvatarTracker
#include "llavataractions.h"
#include "llmenugl.h"
#include "lluictrlfactory.h"

#include "llcachename.h"
#include "llagentdata.h"
#include "llimfloater.h"

#define MENU_ITEM_VIEW_PROFILE 0
#define MENU_ITEM_SEND_IM 1

static LLDefaultChildRegistry::Register<LLAvatarIconCtrl> r("avatar_icon");

bool LLAvatarIconIDCache::LLAvatarIconIDCacheItem::expired()
{
	const F64 SEC_PER_DAY_PLUS_HOUR = (24.0 + 1.0) * 60.0 * 60.0;
	F64 delta = LLDate::now().secondsSinceEpoch() - cached_time.secondsSinceEpoch();
	if (delta > SEC_PER_DAY_PLUS_HOUR)
		return true;
	return false;
}

void LLAvatarIconIDCache::load	()
{
	llinfos << "Loading avatar icon id cache." << llendl;
	
	// build filename for each user
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, mFilename);
	llifstream file(resolved_filename);

	if (!file.is_open())
		return;
	
	// add each line in the file to the list
	int uuid_len = UUID_STR_LENGTH-1;
	std::string line;
	while (std::getline(file, line)) 
	{
		LLUUID avatar_id;
		LLUUID icon_id;
		LLDate date;

		std::string avatar_id_str = line.substr(0,uuid_len);
		std::string icon_id_str = line.substr(uuid_len,uuid_len);
		
		std::string date_str = line.substr(uuid_len*2, line.length()-uuid_len*2);

		if(!avatar_id.set(avatar_id_str) || !icon_id.set(icon_id_str) || !date.fromString(date_str))
			continue;

		LLAvatarIconIDCacheItem item = {icon_id,date};
		mCache[avatar_id] = item;
	}

	file.close();
	
}

void LLAvatarIconIDCache::save	()
{
	std::string resolved_filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, mFilename);

	// open a file for writing
	llofstream file (resolved_filename);
	if (!file.is_open())
	{
		llwarns << "can't open avatar icons cache file\"" << mFilename << "\" for writing" << llendl;
		return;
	}

	for(std::map<LLUUID,LLAvatarIconIDCacheItem>::iterator it = mCache.begin();it!=mCache.end();++it)
	{
		if(!it->second.expired())
		{
			file << it->first << it->second.icon_id << it->second.cached_time << std::endl;	
		}
	}
	
	file.close();
}

LLUUID*	LLAvatarIconIDCache::get		(const LLUUID& avatar_id)
{
	std::map<LLUUID,LLAvatarIconIDCacheItem>::iterator it = mCache.find(avatar_id);
	if(it==mCache.end())
		return 0;
	if(it->second.expired())
		return 0;
	return &it->second.icon_id;
}

void LLAvatarIconIDCache::add		(const LLUUID& avatar_id,const LLUUID& icon_id)
{
	LLAvatarIconIDCacheItem item = {icon_id,LLDate::now()};
	mCache[avatar_id] = item;
}

void LLAvatarIconIDCache::remove	(const LLUUID& avatar_id)
{
	mCache.erase(avatar_id);
}


LLAvatarIconCtrl::Params::Params()
:	avatar_id("avatar_id"),
	draw_tooltip("draw_tooltip", true),
	default_icon_name("default_icon_name")
{
	name = "avatar_icon";
}


LLAvatarIconCtrl::LLAvatarIconCtrl(const LLAvatarIconCtrl::Params& p)
:	LLIconCtrl(p),
	mDrawTooltip(p.draw_tooltip),
	mDefaultIconName(p.default_icon_name)
{
	mPriority = LLViewerFetchedTexture::BOOST_ICON;
	
	LLRect rect = p.rect;
	mDrawWidth  = llmax(32, rect.getWidth()) ;
	mDrawHeight = llmax(32, rect.getHeight()) ;

	static LLUICachedControl<S32> llavatariconctrl_symbol_hpad("UIAvatariconctrlSymbolHPad", 2);
	static LLUICachedControl<S32> llavatariconctrl_symbol_vpad("UIAvatariconctrlSymbolVPad", 2);
	static LLUICachedControl<S32> llavatariconctrl_symbol_size("UIAvatariconctrlSymbolSize", 5);
	static LLUICachedControl<std::string> llavatariconctrl_symbol_pos("UIAvatariconctrlSymbolPosition", "BottomRight");

	// BottomRight is the default position
	S32 left = rect.getWidth() - llavatariconctrl_symbol_size - llavatariconctrl_symbol_hpad;
	S32 bottom = llavatariconctrl_symbol_vpad;

	if ("BottomLeft" == (std::string)llavatariconctrl_symbol_pos)
	{
		left = llavatariconctrl_symbol_hpad;
		bottom = llavatariconctrl_symbol_vpad;
	}
	else if ("TopLeft" == (std::string)llavatariconctrl_symbol_pos)
	{
		left = llavatariconctrl_symbol_hpad;
		bottom = rect.getHeight() - llavatariconctrl_symbol_size - llavatariconctrl_symbol_vpad;
	}
	else if ("TopRight" == (std::string)llavatariconctrl_symbol_pos)
	{
		left = rect.getWidth() - llavatariconctrl_symbol_size - llavatariconctrl_symbol_hpad;
		bottom = rect.getHeight() - llavatariconctrl_symbol_size - llavatariconctrl_symbol_vpad;
	}

	rect.setOriginAndSize(left, bottom, llavatariconctrl_symbol_size, llavatariconctrl_symbol_size);

	if (p.avatar_id.isProvided())
	{
		LLSD value(p.avatar_id);
		setValue(value);
	}
	else
	{
		LLIconCtrl::setValue(mDefaultIconName);
	}
}

LLAvatarIconCtrl::~LLAvatarIconCtrl()
{
	if (mAvatarId.notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId, this);
		// Name callbacks will be automatically disconnected since LLUICtrl is trackable
	}
}

//virtual
void LLAvatarIconCtrl::setValue(const LLSD& value)
{
	if (value.isUUID())
	{
		LLAvatarPropertiesProcessor* app =
			LLAvatarPropertiesProcessor::getInstance();
		if (mAvatarId.notNull())
		{
			app->removeObserver(mAvatarId, this);
		}

		if (mAvatarId != value.asUUID())
		{
			mAvatarId = value.asUUID();

			// *BUG: This will return stale icons if a user changes their
			// profile picture. However, otherwise we send too many upstream
			// AvatarPropertiesRequest messages.

			// to get fresh avatar icon use
			// LLAvatarIconIDCache::getInstance()->remove(avatar_id);

			// Check if cache already contains image_id for that avatar
			if (!updateFromCache())
			{
				LLIconCtrl::setValue(mDefaultIconName);
				app->addObserver(mAvatarId, this);
				app->sendAvatarPropertiesRequest(mAvatarId);
			}
		}
	}
	else
	{
		LLIconCtrl::setValue(value);
	}

	gCacheName->get(mAvatarId, false,
		boost::bind(&LLAvatarIconCtrl::nameUpdatedCallback, this, _1, _2, _3));
}

bool LLAvatarIconCtrl::updateFromCache()
{
	LLUUID* icon_id_ptr = LLAvatarIconIDCache::getInstance()->get(mAvatarId);
	if(!icon_id_ptr)
		return false;

	const LLUUID& icon_id = *icon_id_ptr;

	// Update the avatar
	if (icon_id.notNull())
	{
		LLIconCtrl::setValue(icon_id);
	}
	else
	{
		LLIconCtrl::setValue(mDefaultIconName);
	}

	return true;
}

//virtual
void LLAvatarIconCtrl::processProperties(void* data, EAvatarProcessorType type)
{
	if (APT_PROPERTIES == type)
	{
		LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);
		if (avatar_data)
		{
			if (avatar_data->avatar_id != mAvatarId)
			{
				return;
			}

			LLAvatarIconIDCache::getInstance()->add(mAvatarId,avatar_data->image_id);
			updateFromCache();
		}
	}
}

void LLAvatarIconCtrl::nameUpdatedCallback(
	const LLUUID& id,
	const std::string& name,
	bool is_group)
{
	if (id == mAvatarId)
	{
		mFullName = name;

		if (mDrawTooltip)
		{
			setToolTip(name);
		}
		else
		{
			setToolTip(std::string());
		}
	}
}
