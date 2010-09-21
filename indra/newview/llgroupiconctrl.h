/** 
 * @file llgroupiconctrl.h
 * @brief LLGroupIconCtrl class declaration
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
