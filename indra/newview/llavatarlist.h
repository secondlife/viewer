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

class LLTimer;

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
class LLAvatarList : public LLFlatListView
{
	LOG_CLASS(LLAvatarList);
public:
	typedef uuid_vec_t uuid_vector_t;

	struct Params : public LLInitParam::Block<Params, LLFlatListView::Params> 
	{
		Optional<bool>	ignore_online_status, // show all items as online
						show_last_interaction_time, // show most recent interaction time. *HACK: move this to a derived class
						show_info_btn,
						show_profile_btn,
						show_speaking_indicator;
		Params();
	};

	LLAvatarList(const Params&);
	virtual	~LLAvatarList();

	virtual void draw(); // from LLView

	virtual void clear();

	void setNameFilter(const std::string& filter);
	void setDirty(bool val = true, bool force_refresh = false);
	uuid_vector_t& getIDs() 							{ return mIDs; }
	bool contains(const LLUUID& id);

	void setContextMenu(LLAvatarListItem::ContextMenu* menu) { mContextMenu = menu; }
	void setSessionID(const LLUUID& session_id) { mSessionID = session_id; }
	const LLUUID& getSessionID() { return mSessionID; }

	void toggleIcons();
	void setSpeakingIndicatorsVisible(bool visible);
	void sortByName();
	void setShowIcons(std::string param_name);
	bool getIconsVisible() const { return mShowIcons; }
	const std::string getIconParamName() const{return mIconParamName;}
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	// Return true if filter has at least one match.
	bool filterHasMatches();

	boost::signals2::connection setRefreshCompleteCallback(const commit_signal_t::slot_type& cb);

	boost::signals2::connection setItemDoubleClickCallback(const mouse_signal_t::slot_type& cb);

protected:
	void refresh();

	void addNewItem(const LLUUID& id, const std::string& name, BOOL is_online, EAddPosition pos = ADD_BOTTOM);
	void computeDifference(
		const uuid_vec_t& vnew,
		uuid_vec_t& vadded,
		uuid_vec_t& vremoved);
	void updateLastInteractionTimes();
	void onItemDoucleClicked(LLUICtrl* ctrl, S32 x, S32 y, MASK mask);

private:

	bool mIgnoreOnlineStatus;
	bool mShowLastInteractionTime;
	bool mDirty;
	bool mShowIcons;
	bool mShowInfoBtn;
	bool mShowProfileBtn;
	bool mShowSpeakingIndicator;

	LLTimer*				mLITUpdateTimer; // last interaction time update timer
	std::string				mIconParamName;
	std::string				mNameFilter;
	uuid_vector_t			mIDs;
	LLUUID					mSessionID;

	LLAvatarListItem::ContextMenu* mContextMenu;

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

#endif // LL_LLAVATARLIST_H
