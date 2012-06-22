/**
 * @file llblocklist.h
 * @brief List of the blocked avatars and objects.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LLBLOCKLIST_H_
#define LLBLOCKLIST_H_

#include "llflatlistview.h"
#include "lllistcontextmenu.h"
#include "llmutelist.h"
#include "lltoggleablemenu.h"

class LLBlockedListItem;
class LLMute;

/**
 * List of blocked avatars and objects.
 * This list represents contents of the LLMuteList.
 * Each change in LLMuteList leads to rebuilding this list, so
 * it's always in actual state.
 */
class LLBlockList: public LLFlatListViewEx, public LLMuteListObserver
{
	LOG_CLASS(LLBlockList);
public:
	struct Params : public LLInitParam::Block<Params, LLFlatListViewEx::Params>
	{
		Params(){};
	};

	LLBlockList(const Params& p);
	virtual ~LLBlockList();

	virtual BOOL 		handleRightMouseDown(S32 x, S32 y, MASK mask);
	LLToggleableMenu*	getContextMenu() const { return mContextMenu.get(); }
	LLBlockedListItem*	getBlockedItem() const;

	virtual void onChange() { refresh(); }
	virtual void draw();

	void setNameFilter(const std::string& filter);
	void sortByName();
	void sortByType();
	void refresh();

private:

	void addNewItem(const LLMute* mute);
	void setDirty(bool dirty = true) { mDirty = dirty; }
	bool findInsensitive(std::string haystack, const std::string& needle_upper);

	bool isActionEnabled(const LLSD& userdata);
	void onCustomAction (const LLSD& userdata);


	LLHandle<LLToggleableMenu>	mContextMenu;

	LLBlockedListItem*			mSelectedItem;
	std::string 				mNameFilter;
	bool 						mDirty;

};


/*
 * Abstract comparator for blocked items
 */
class LLBlockListItemComparator : public LLFlatListView::ItemComparator
{
	LOG_CLASS(LLBlockListItemComparator);

public:
	LLBlockListItemComparator() {};
	virtual ~LLBlockListItemComparator() {};

	virtual bool compare(const LLPanel* item1, const LLPanel* item2) const;

protected:

	virtual bool doCompare(const LLBlockedListItem* blocked_item1, const LLBlockedListItem* blocked_item2) const = 0;
};


/*
 * Compares items by name
 */
class LLBlockListNameComparator : public LLBlockListItemComparator
{
	LOG_CLASS(LLBlockListNameComparator);

public:
	LLBlockListNameComparator() {};
	virtual ~LLBlockListNameComparator() {};

protected:

	virtual bool doCompare(const LLBlockedListItem* blocked_item1, const LLBlockedListItem* blocked_item2) const;
};

/*
 * Compares items by type and then by name within type
 * Objects come first then avatars
 */
class LLBlockListNameTypeComparator : public LLBlockListItemComparator
{
	LOG_CLASS(LLBlockListNameTypeComparator);

public:
	LLBlockListNameTypeComparator() {};
	virtual ~LLBlockListNameTypeComparator() {};

protected:

	virtual bool doCompare(const LLBlockedListItem* blocked_item1, const LLBlockedListItem* blocked_item2) const;
};

#endif /* LLBLOCKLIST_H_ */
