/** 
 * @file llavatariconctrl.h
 * @brief LLAvatarIconCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLAVATARICONCTRL_H
#define LL_LLAVATARICONCTRL_H

#include "lliconctrl.h"
#include "llavatarpropertiesprocessor.h"
#include "llviewermenu.h"

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

	void nameUpdatedCallback(
		const LLUUID& id,
		const std::string& name,
		bool is_group);

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
