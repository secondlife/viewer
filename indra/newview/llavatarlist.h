/** 
 * @file llavatarlist.h
 * @brief Generic avatar list
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

#ifndef LL_LLAVATARLIST_H
#define LL_LLAVATARLIST_H

#include <llscrolllistctrl.h>

// *TODO: derive from ListView when it's ready.
class LLAvatarList : public LLScrollListCtrl 
{
	LOG_CLASS(LLAvatarList);
public:
	struct Params : public LLInitParam::Block<Params, LLScrollListCtrl::Params>
	{
		Params();
	};

	enum AVATAR_LIST_COLUMN_ORDER
	{
		LIST_NAME,
	};

	LLAvatarList(const Params&);
	virtual	~LLAvatarList() {}

	BOOL updateList(const std::vector<LLUUID>& all_buddies);

protected:
	std::vector<LLUUID> getSelectedIDs();
	void addItem(const LLUUID& id, const std::string& name, BOOL is_bold, EAddPosition pos = ADD_BOTTOM);
};

#endif // LL_LLAVATARLIST_H
