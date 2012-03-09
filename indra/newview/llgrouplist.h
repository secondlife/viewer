/** 
 * @file llgrouplist.h
 * @brief List of the groups the agent belongs to.
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

#ifndef LL_LLGROUPLIST_H
#define LL_LLGROUPLIST_H

#include "llevent.h"
#include "llflatlistview.h"
#include "llpanel.h"
#include "llpointer.h"
#include "llstyle.h"
#include "llgroupmgr.h"

/**
 * Auto-updating list of agent groups.
 * 
 * Can use optional group name filter.
 * 
 * @see setNameFilter()
 */
class LLGroupList: public LLFlatListViewEx, public LLOldEvents::LLSimpleListener
{
	LOG_CLASS(LLGroupList);
public:

	LLGroupList(const Params& p);
	virtual ~LLGroupList();

	virtual void draw(); // from LLView
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask); // from LLView
	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask); // from LLView

	void setNameFilter(const std::string& filter);
	void toggleIcons();
	bool getIconsVisible() const { return mShowIcons; }

private:
	void setDirty(bool val = true)		{ mDirty = val; }
	void refresh();
	void addNewItem(const LLUUID& id, const std::string& name, const LLUUID& icon_id, EAddPosition pos = ADD_BOTTOM);
	bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata); // called on agent group list changes

	bool onContextMenuItemClick(const LLSD& userdata);
	bool onContextMenuItemEnable(const LLSD& userdata);

	LLHandle<LLView>	mContextMenuHandle;

	bool mShowIcons;
	bool mDirty;
	std::string mNameFilter;
};

class LLButton;
class LLIconCtrl;
class LLTextBox;

class LLGroupListItem : public LLPanel
	, public LLGroupMgrObserver
{
public:
	LLGroupListItem();
	~LLGroupListItem();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setValue(const LLSD& value);
	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);

	const LLUUID& getGroupID() const			{ return mGroupID; }
	const std::string& getGroupName() const		{ return mGroupName; }

	void setName(const std::string& name, const std::string& highlight = LLStringUtil::null);
	void setGroupID(const LLUUID& group_id);
	void setGroupIconID(const LLUUID& group_icon_id);
	void setGroupIconVisible(bool visible);

	virtual void changed(LLGroupChange gc);
private:
	void setActive(bool active);
	void onInfoBtnClick();
	void onProfileBtnClick();

	LLTextBox*	mGroupNameBox;
	LLUUID		mGroupID;
	LLIconCtrl* mGroupIcon;
	LLButton*	mInfoBtn;

	std::string	mGroupName;
	LLStyle::Params mGroupNameStyle;

	static S32	sIconWidth; // icon width + padding
};
#endif // LL_LLGROUPLIST_H
