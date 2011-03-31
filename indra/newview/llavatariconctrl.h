/** 
 * @file llavatariconctrl.h
 * @brief LLAvatarIconCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLAVATARICONCTRL_H
#define LL_LLAVATARICONCTRL_H

#include "lliconctrl.h"
#include "llavatarpropertiesprocessor.h"
#include "llviewermenu.h"

class LLAvatarName;

class LLAvatarIconIDCache: public LLSingleton<LLAvatarIconIDCache>
{
public:
	struct LLAvatarIconIDCacheItem
	{
		LLUUID icon_id;
		LLDate cached_time;	

		bool expired();
	};

	LLAvatarIconIDCache():mFilename("avatar_icons_cache.txt")
	{
	}

	void				load	();
	void				save	();

	LLUUID*				get		(const LLUUID& id);
	void				add		(const LLUUID& avatar_id,const LLUUID& icon_id);

	void				remove	(const LLUUID& id);
protected:
	

	std::string	mFilename;
	std::map<LLUUID,LLAvatarIconIDCacheItem> mCache;//we cache only LLUID and time
};

class LLAvatarIconCtrl
: public LLIconCtrl, public LLAvatarPropertiesObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLIconCtrl::Params>
	{
		Optional <LLUUID> avatar_id;
		Optional <bool> draw_tooltip;
		Optional <std::string> default_icon_name;
		Params();
	};
	
protected:
	LLAvatarIconCtrl(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLAvatarIconCtrl();

	virtual void setValue(const LLSD& value);

	// LLAvatarPropertiesProcessor observer trigger
	virtual void processProperties(void* data, EAvatarProcessorType type);

	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

	const LLUUID&		getAvatarId() const	{ return mAvatarId; }
	const std::string&	getFullName() const { return mFullName; }

	void setDrawTooltip(bool value) { mDrawTooltip = value;}

protected:
	LLUUID				mAvatarId;
	std::string			mFullName;
	bool				mDrawTooltip;
	std::string			mDefaultIconName;

	bool updateFromCache();
};

#endif  // LL_LLAVATARICONCTRL_H
