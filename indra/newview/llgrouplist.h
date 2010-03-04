/** 
 * @file llgrouplist.h
 * @brief List of the groups the agent belongs to.
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
class LLGroupList: public LLFlatListView, public LLOldEvents::LLSimpleListener
{
	LOG_CLASS(LLGroupList);
public:
	struct Params : public LLInitParam::Block<Params, LLFlatListView::Params> 
	{
		/**
		 * Contains a message for empty list when user is not a member of any group
		 */
		Optional<std::string>	no_groups_msg;

		/**
		 * Contains a message for empty list when all groups don't match passed filter
		 */
		Optional<std::string>	no_filtered_groups_msg;
		Params();
	};

	LLGroupList(const Params& p);
	virtual ~LLGroupList();

	virtual void draw(); // from LLView
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask); // from LLView

	void setNameFilter(const std::string& filter);
	void toggleIcons();
	bool getIconsVisible() const { return mShowIcons; }

	// *WORKAROUND: two methods to overload appropriate Params due to localization issue:
	// no_groups_msg & no_filtered_groups_msg attributes are not defined as translatable in VLT. See EXT-5931
	void setNoGroupsMsg(const std::string& msg) { mNoGroupsMsg = msg; }
	void setNoFilteredGroupsMsg(const std::string& msg) { mNoFilteredGroupsMsg = msg; }
	
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
	std::string mNoFilteredGroupsMsg;
	std::string mNoGroupsMsg;
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
