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

#include <boost/signals2.hpp>

#include "lliconctrl.h"
#include "llavatarpropertiesprocessor.h"
#include "llviewermenu.h"

class LLAvatarName;

class LLAvatarIconIDCache: public LLSingleton<LLAvatarIconIDCache>
{
	LLSINGLETON(LLAvatarIconIDCache);

public:
	struct LLAvatarIconIDCacheItem
	{
		LLUUID icon_id;
		LLDate cached_time;

		bool expired();
	};

	void				load	();
	void				save	();

	LLUUID*				get		(const LLUUID& id);
	void				add		(const LLUUID& avatar_id,const LLUUID& icon_id);

	void				remove	(const LLUUID& id);
protected:
	

	std::string	mFilename;
	std::map<LLUUID,LLAvatarIconIDCacheItem> mCache;//we cache only LLUID and time
};

inline
LLAvatarIconIDCache::LLAvatarIconIDCache()
	:	mFilename("avatar_icons_cache.txt")
{}

namespace LLAvatarIconCtrlEnums
{
	enum ESymbolPos
	{
		BOTTOM_LEFT,
		BOTTOM_RIGHT,
		TOP_LEFT,
		TOP_RIGHT
	};
}


namespace LLInitParam
{
	template<>
	struct TypeValues<LLAvatarIconCtrlEnums::ESymbolPos> : public TypeValuesHelper<LLAvatarIconCtrlEnums::ESymbolPos>
	{
		static void declareValues();
	};
}

class LLAvatarIconCtrl
: public LLIconCtrl, public LLAvatarPropertiesObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLIconCtrl::Params>
	{
		Optional<LLUUID>		avatar_id;
		Optional<bool>			draw_tooltip;
		Optional<std::string>	default_icon_name;
		Optional<S32>			symbol_hpad,
								symbol_vpad,
								symbol_size;
		Optional<LLAvatarIconCtrlEnums::ESymbolPos>	symbol_pos;

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

	const LLUUID&		getAvatarId() const	{ return mAvatarId; }
	const std::string&	getFullName() const { return mFullName; }

	void setDrawTooltip(bool value) { mDrawTooltip = value;}

protected:
	LLUUID                      mAvatarId;
	std::string                 mFullName;
	bool                        mDrawTooltip;
	std::string                 mDefaultIconName;
	S32							mSymbolHpad,
								mSymbolVpad,
								mSymbolSize;
	LLAvatarIconCtrlEnums::ESymbolPos	mSymbolPos;

	bool updateFromCache();

private:
	void fetchAvatarName();
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

	boost::signals2::connection mAvatarNameCacheConnection;
};

#endif  // LL_LLAVATARICONCTRL_H
