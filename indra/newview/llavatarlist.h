/** 
 * @file llavatarlist.h
 * @brief Generic avatar list
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

#ifndef LL_LLAVATARLIST_H
#define LL_LLAVATARLIST_H

#include "llflatlistview.h"
#include "llavatarlistitem.h"

class LLTimer;
class LLListContextMenu;

/**
 * Generic list of avatars.
 * 
 * Updates itself when it's dirty, using optional name filter.
 * To initiate update, modify the UUID list and call setDirty().
 * 
 * @see getIDs()
 * @see setDirty()
 * @see setNameFilter()
 */
class LLAvatarList : public LLFlatListViewEx
{
	LOG_CLASS(LLAvatarList);
public:
	struct Params : public LLInitParam::Block<Params, LLFlatListViewEx::Params>
	{
		Optional<bool>	ignore_online_status, // show all items as online
						show_last_interaction_time, // show most recent interaction time. *HACK: move this to a derived class
						show_info_btn,
						show_profile_btn,
						show_speaking_indicator,
						show_permissions_granted;
		Params();
	};

	LLAvatarList(const Params&);
	virtual	~LLAvatarList();

	virtual void draw(); // from LLView

	virtual void clear();

	virtual void setVisible(BOOL visible);

	void setNameFilter(const std::string& filter);
	void setDirty(bool val = true, bool force_refresh = false);
	uuid_vec_t& getIDs() 							{ return mIDs; }
	bool contains(const LLUUID& id);

	void setContextMenu(LLListContextMenu* menu) { mContextMenu = menu; }
	void setSessionID(const LLUUID& session_id) { mSessionID = session_id; }
	const LLUUID& getSessionID() { return mSessionID; }

	void toggleIcons();
	void setSpeakingIndicatorsVisible(bool visible);
	void showPermissions(bool visible);
	void sortByName();
	void setShowIcons(std::string param_name);
	bool getIconsVisible() const { return mShowIcons; }
	const std::string getIconParamName() const{return mIconParamName;}
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	// Return true if filter has at least one match.
	bool filterHasMatches();

	boost::signals2::connection setRefreshCompleteCallback(const commit_signal_t::slot_type& cb);

	boost::signals2::connection setItemDoubleClickCallback(const mouse_signal_t::slot_type& cb);

	virtual S32 notifyParent(const LLSD& info);

	void addAvalineItem(const LLUUID& item_id, const LLUUID& session_id, const std::string& item_name);
	void handleDisplayNamesOptionChanged();

protected:
	void refresh();

	void addNewItem(const LLUUID& id, const std::string& name, BOOL is_online, EAddPosition pos = ADD_BOTTOM);
	void computeDifference(
		const uuid_vec_t& vnew,
		uuid_vec_t& vadded,
		uuid_vec_t& vremoved);
	void updateLastInteractionTimes();	
	void rebuildNames();
	void onItemDoubleClicked(LLUICtrl* ctrl, S32 x, S32 y, MASK mask);
	void updateAvatarNames();

private:

	bool mIgnoreOnlineStatus;
	bool mShowLastInteractionTime;
	bool mDirty;
	bool mNeedUpdateNames;
	bool mShowIcons;
	bool mShowInfoBtn;
	bool mShowProfileBtn;
	bool mShowSpeakingIndicator;
	bool mShowPermissions;

	LLTimer*				mLITUpdateTimer; // last interaction time update timer
	std::string				mIconParamName;
	std::string				mNameFilter;
	uuid_vec_t				mIDs;
	LLUUID					mSessionID;

	LLListContextMenu*	mContextMenu;

	commit_signal_t mRefreshCompleteSignal;
	mouse_signal_t mItemDoubleClickSignal;
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

class LLAvatarItemAgentOnTopComparator : public LLAvatarItemNameComparator
{
	LOG_CLASS(LLAvatarItemAgentOnTopComparator);

public:
	LLAvatarItemAgentOnTopComparator() {};
	virtual ~LLAvatarItemAgentOnTopComparator() {};

protected:
	virtual bool doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const;
};

/**
 * Represents Avaline caller in Avatar list in Voice Control Panel and group chats.
 */
class LLAvalineListItem : public LLAvatarListItem
{
public:

	/**
	 * Constructor
	 *
	 * @param hide_number - flag indicating if number should be hidden.
	 *		In this case It will be shown as "Avaline Caller 1", "Avaline Caller 1", etc.
	 */
	LLAvalineListItem(bool hide_number = true);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void setName(const std::string& name);

private:
	bool mIsHideNumber;
};

#endif // LL_LLAVATARLIST_H
