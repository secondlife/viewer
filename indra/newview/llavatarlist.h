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

#include "llflatlistview.h"

#include "llavatarlistitem.h"

class LLAvatarList : public LLFlatListView
{
	LOG_CLASS(LLAvatarList);
public:
	struct Params : public LLInitParam::Block<Params, LLFlatListView::Params> 
	{
		Optional<S32> volume_column_width;
		Optional<bool> online_go_first;
		Params();
	};

	LLAvatarList(const Params&);
	virtual	~LLAvatarList() {}

	BOOL update(const std::vector<LLUUID>& all_buddies,
		const std::string& name_filter = LLStringUtil::null);

	void setContextMenu(LLAvatarListItem::ContextMenu* menu) { mContextMenu = menu; }

	void sortByName();

protected:
	void addNewItem(const LLUUID& id, const std::string& name, BOOL is_bold, EAddPosition pos = ADD_BOTTOM);
	void computeDifference(
		const std::vector<LLUUID>& vnew,
		std::vector<LLUUID>& vadded,
		std::vector<LLUUID>& vremoved);

private:

	bool mOnlineGoFirst;

	LLAvatarListItem::ContextMenu* mContextMenu;
};

/** Abstract comparator for avatar items */
class LLAvatarItemComparator : public LLFlatListView::ItemComparator
{
	LOG_CLASS(LLAvatarItemComparator);

public:
	LLAvatarItemComparator() {};
	virtual ~LLAvatarItemComparator() {};

	virtual bool compare(const LLPanel* item1, const LLPanel* item2) const;

protected:

	/** 
	 * Returns true if avatar_item1 < avatar_item2, false otherwise 
	 * Implement this method in your particular comparator.
	 * In Linux a compiler failed to build it using the name "compare", so it was renamed to doCompare
	 */
	virtual bool doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const = 0;
};


class LLAvatarItemNameComparator : public LLAvatarItemComparator
{
	LOG_CLASS(LLAvatarItemNameComparator);

public:
	LLAvatarItemNameComparator() {};
	virtual ~LLAvatarItemNameComparator() {};

protected:
	virtual bool doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const;
};

#endif // LL_LLAVATARLIST_H
