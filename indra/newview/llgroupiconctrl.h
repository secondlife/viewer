/** 
 * @file llgroupiconctrl.h
 * @brief LLGroupIconCtrl class declaration
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

#ifndef LL_LLGROUPICONCTRL_H
#define LL_LLGROUPICONCTRL_H

#include "lliconctrl.h"

#include "llgroupmgr.h"

/**
 * Extends IconCtrl to show group icon wherever it is needed.
 * 
 * It gets icon id by group id from the LLGroupMgr.
 * If group data is not loaded yet it subscribes as LLGroupMgr observer and requests necessary data.
 */
class LLGroupIconCtrl
	: public LLIconCtrl, public LLGroupMgrObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLIconCtrl::Params>
	{
		Optional <LLUUID> group_id;
		Optional <bool> draw_tooltip;
		Optional <std::string> default_icon_name;
		Params();
	};

protected:
	LLGroupIconCtrl(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLGroupIconCtrl();

	/**
	 * Determines group icon id by group id and sets it as icon value.
	 *
	 * Icon id is got from the appropriate LLGroupMgrGroupData specified by group UUID.
	 * If necessary it requests necessary data from the LLGroupMgr.
	 *
	 * @params value - if LLUUID - it is processed as group id otherwise base method is called.
	 */
	virtual void setValue(const LLSD& value);

	/**
	 * Sets icon_id as icon value directly. Avoids LLGroupMgr cache checks for group id
	 * Uses default icon in case id is null.
	 *
	 * @params icon_id - it is processed as icon id, default image will be used in case id is null.
	 */
	void setIconId(const LLUUID& icon_id);

	// LLGroupMgrObserver observer trigger
	virtual void changed(LLGroupChange gc);

	const std::string&	getGroupName() const { return mGroupName; }
	void setDrawTooltip(bool value) { mDrawTooltip = value;}

	const LLUUID&		getGroupId() const	{ return mGroupId; }

protected:
	LLUUID				mGroupId;
	std::string			mGroupName;
	bool				mDrawTooltip;
	std::string			mDefaultIconName;

	bool updateFromCache();
};

#endif  // LL_LLGROUPICONCTRL_H
